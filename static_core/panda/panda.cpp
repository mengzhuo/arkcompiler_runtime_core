/**
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "include/runtime.h"
#include "include/thread.h"
#include "include/thread_scopes.h"
#include "runtime/include/locks.h"
#include "runtime/include/method-inl.h"
#include "runtime/include/class.h"
#include "utils/pandargs.h"
#include "compiler/compiler_options.h"
#include "compiler/compiler_logger.h"
#include "compiler_events_gen.h"
#include "mem/mem_stats.h"
#include "libpandabase/os/mutex.h"
#include "libpandabase/os/native_stack.h"
#include "generated/base_options.h"

#include "ark_version.h"

#include "utils/span.h"

#include "utils/logger.h"

#include <limits>
#include <iostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <csignal>

namespace panda {
const panda_file::File *GetPandaFile(const ClassLinker &classLinker, std::string_view fileName)
{
    const panda_file::File *res = nullptr;
    classLinker.EnumerateBootPandaFiles([&res, fileName](const panda_file::File &pf) {
        if (pf.GetFilename() == fileName) {
            res = &pf;
            return false;
        }
        return true;
    });
    return res;
}

void PrintHelp(const panda::PandArgParser &paParser)
{
    std::cerr << paParser.GetErrorString() << std::endl;
    std::cerr << "Usage: "
              << "panda"
              << " [OPTIONS] [file] [entrypoint] -- [arguments]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "optional arguments:" << std::endl;
    std::cerr << paParser.GetHelpString() << std::endl;
}

bool PrepareArguments(panda::PandArgParser *paParser, const RuntimeOptions &runtimeOptions,
                      const panda::PandArg<std::string> &file, const panda::PandArg<std::string> &entrypoint,
                      const panda::PandArg<bool> &help, int argc, const char **argv)
{
    auto startTime =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    if (!paParser->Parse(argc, argv)) {
        PrintHelp(*paParser);
        return false;
    }

    if (runtimeOptions.IsVersion()) {
        PrintPandaVersion();
        return false;
    }

    if (file.GetValue().empty() || entrypoint.GetValue().empty() || help.GetValue()) {
        PrintHelp(*paParser);
        return false;
    }

    if (runtimeOptions.IsStartupTime()) {
        std::cout << "\n"
                  << "Startup start time: " << startTime << std::endl;
    }

    auto runtimeOptionsErr = runtimeOptions.Validate();
    if (runtimeOptionsErr) {
        std::cerr << "Error: " << runtimeOptionsErr.value().GetMessage() << std::endl;
        return false;
    }

    auto compilerOptionsErr = compiler::g_options.Validate();
    if (compilerOptionsErr) {
        std::cerr << "Error: " << compilerOptionsErr.value().GetMessage() << std::endl;
        return false;
    }

    return true;
}

int Main(int argc, const char **argv)
{
    Span<const char *> sp(argv, argc);
    RuntimeOptions runtimeOptions(sp[0]);
    base_options::Options baseOptions(sp[0]);
    panda::PandArgParser paParser;

    panda::PandArg<bool> help("help", false, "Print this message and exit");
    panda::PandArg<bool> options("options", false, "Print compiler and runtime options");
    // tail arguments
    panda::PandArg<std::string> file("file", "", "path to pandafile");
    panda::PandArg<std::string> entrypoint("entrypoint", "", "full name of entrypoint function or method");

    runtimeOptions.AddOptions(&paParser);
    baseOptions.AddOptions(&paParser);
    compiler::g_options.AddOptions(&paParser);

    paParser.Add(&help);
    paParser.Add(&options);
    paParser.PushBackTail(&file);
    paParser.PushBackTail(&entrypoint);
    paParser.EnableTail();
    paParser.EnableRemainder();

    if (!panda::PrepareArguments(&paParser, runtimeOptions, file, entrypoint, help, argc, argv)) {
        return 1;
    }

    compiler::g_options.AdjustCpuFeatures(false);

    Logger::Initialize(baseOptions);

    runtimeOptions.SetVerificationMode(VerificationModeFromString(
        static_cast<Options>(runtimeOptions).GetVerificationMode()));  // NOLINT(cppcoreguidelines-slicing)
    if (runtimeOptions.IsVerificationEnabled()) {
        if (!runtimeOptions.WasSetVerificationMode()) {
            runtimeOptions.SetVerificationMode(VerificationMode::AHEAD_OF_TIME);
        }
    }

    arg_list_t arguments = paParser.GetRemainder();

    panda::compiler::CompilerLogger::SetComponents(panda::compiler::g_options.GetCompilerLog());
    if (compiler::g_options.IsCompilerEnableEvents()) {
        panda::compiler::EventWriter::Init(panda::compiler::g_options.GetCompilerEventsPath());
    }

    auto bootPandaFiles = runtimeOptions.GetBootPandaFiles();

    if (runtimeOptions.GetPandaFiles().empty()) {
        bootPandaFiles.push_back(file.GetValue());
    } else {
        auto pandaFiles = runtimeOptions.GetPandaFiles();
        auto foundIter = std::find_if(pandaFiles.begin(), pandaFiles.end(),
                                      [&](auto &fileName) { return fileName == file.GetValue(); });
        if (foundIter == pandaFiles.end()) {
            pandaFiles.push_back(file.GetValue());
            runtimeOptions.SetPandaFiles(pandaFiles);
        }
    }

    runtimeOptions.SetBootPandaFiles(bootPandaFiles);

    if (!Runtime::Create(runtimeOptions)) {
        std::cerr << "Error: cannot create runtime" << std::endl;
        return -1;
    }

    int ret = 0;

    if (options.GetValue()) {
        std::cout << paParser.GetRegularArgs() << std::endl;
    }

    std::string fileName = file.GetValue();
    std::string entry = entrypoint.GetValue();

    auto &runtime = *Runtime::GetCurrent();

    auto res = runtime.ExecutePandaFile(fileName, entry, arguments);
    if (!res) {
        std::cerr << "Cannot execute panda file '" << fileName << "' with entry '" << entry << "'" << std::endl;
        ret = -1;
    } else {
        ret = res.Value();
    }

    if (runtimeOptions.IsPrintMemoryStatistics()) {
        std::cout << runtime.GetMemoryStatistics();
    }
    if (runtimeOptions.IsPrintGcStatistics()) {
        std::cout << runtime.GetFinalStatistics();
    }
    if (!Runtime::Destroy()) {
        std::cerr << "Error: cannot destroy runtime" << std::endl;
        return -1;
    }
    paParser.DisableTail();
    return ret;
}
}  // namespace panda

int main(int argc, const char **argv)
{
    return panda::Main(argc, argv);
}
