/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "optimizer/ir/graph.h"
#include "compiler_options.h"

#include "llvm_compiler.h"
#include "llvm_ark_interface.h"
#include "llvm_logger.h"
#include "llvm_options.h"

#include <llvm/Config/llvm-config.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/InitializePasses.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/StringSaver.h>

namespace panda::llvmbackend {

static llvm::llvm_shutdown_obj g_shutdown {};

LLVMCompiler::LLVMCompiler(Arch arch) : arch_(arch)
{
#ifndef REQUIRED_LLVM_VERSION
#error Internal build error, missing cmake variable
#else
#define STRINGIFY(s) STR(s)  // NOLINT(cppcoreguidelines-macro-usage)
#define STR(s) #s            // NOLINT(cppcoreguidelines-macro-usage)
    // LLVM_VERSION_STRING is defined in llvm-config.h
    // REQUIRED_LLVM_VERSION is defined in libllvmbackend/CMakeLists.txt
    static_assert(std::string_view {LLVM_VERSION_STRING} == STRINGIFY(REQUIRED_LLVM_VERSION));

    const std::string currentLlvmLibVersion = llvm::LLVMContext::getLLVMVersion();
    if (currentLlvmLibVersion != STRINGIFY(REQUIRED_LLVM_VERSION)) {
        llvm::report_fatal_error(llvm::Twine("Incompatible LLVM version " + currentLlvmLibVersion + ". " +
                                             std::string(STRINGIFY(REQUIRED_LLVM_VERSION)) + " is required."),
                                 false); /* gen_crash_diag = false */
    }
#undef STR
#undef STRINGIFY
#endif
    InitializeLLVMTarget(arch);
    InitializeLLVMPasses();
    InitializeLLVMOptions();
#ifdef NDEBUG
    context_.setDiscardValueNames(true);
#endif
    context_.setOpaquePointers(true);

    LLVMLogger::Init(g_options.GetLlvmLog());
}

bool LLVMCompiler::IsInliningDisabled()
{
    if (panda::compiler::g_options.WasSetCompilerInlining() && !g_options.WasSetLlvmInlining()) {
        return !panda::compiler::g_options.IsCompilerInlining();
    }
    return !g_options.IsLlvmInlining();
}

bool LLVMCompiler::IsInliningDisabled(compiler::Graph *graph)
{
    ASSERT(graph != nullptr);
    return IsInliningDisabled(graph->GetRuntime(), graph->GetMethod());
}

bool LLVMCompiler::IsInliningDisabled(compiler::RuntimeInterface *runtime, compiler::RuntimeInterface::MethodPtr method)
{
    ASSERT(runtime != nullptr);
    ASSERT(method != nullptr);

    if (IsInliningDisabled()) {
        return true;
    }

    auto skipList = panda::compiler::g_options.GetCompilerInliningBlacklist();
    if (!skipList.empty()) {
        std::string methodName = runtime->GetMethodFullName(method);
        if (std::find(skipList.begin(), skipList.end(), methodName) != skipList.end()) {
            return true;
        }
    }

    return (runtime->GetMethodName(method).find("__noinline__") != std::string::npos);
}

panda::llvmbackend::LLVMCompilerOptions LLVMCompiler::InitializeLLVMCompilerOptions()
{
    panda::llvmbackend::LLVMCompilerOptions llvmCompilerOptions {};
    llvmCompilerOptions.optimize = !panda::compiler::g_options.IsCompilerNonOptimizing();
    llvmCompilerOptions.optlevel = llvmCompilerOptions.optimize ? 2U : 0U;
    llvmCompilerOptions.gcIntrusionChecks = g_options.IsLlvmGcCheck();
    llvmCompilerOptions.useSafepoint = panda::compiler::g_options.IsCompilerUseSafepoint();
    llvmCompilerOptions.dumpModuleAfterOptimizations = g_options.IsLlvmDumpAfter();
    llvmCompilerOptions.dumpModuleBeforeOptimizations = g_options.IsLlvmDumpBefore();
    llvmCompilerOptions.inlineModuleFile = g_options.GetLlvmInlineModule();
    llvmCompilerOptions.pipelineFile = g_options.GetLlvmPipeline();

    llvmCompilerOptions.inlining = !IsInliningDisabled();
    llvmCompilerOptions.recursiveInlining = g_options.IsLlvmRecursiveInlining();
    llvmCompilerOptions.doIrtocInline = !llvmCompilerOptions.inlineModuleFile.empty();

    return llvmCompilerOptions;
}

void LLVMCompiler::InitializeDefaultLLVMOptions()
{
    if (panda::compiler::g_options.IsCompilerNonOptimizing()) {
        constexpr auto DISABLE = llvm::cl::boolOrDefault::BOU_FALSE;
        SetLLVMOption("fast-isel", DISABLE);
        SetLLVMOption("global-isel", DISABLE);
    }
}

void LLVMCompiler::InitializeLLVMOptions()
{
    llvm::cl::ResetAllOptionOccurrences();
    InitializeDefaultLLVMOptions();
    auto llvmOptionsStr = g_options.GetLlvmOptions();
    if (llvmOptionsStr.length() == 0) {
        return;
    }

    llvm::BumpPtrAllocator alloc;
    llvm::StringSaver stringSaver(alloc);
    llvm::SmallVector<const char *, 0> parsedArgv;
    parsedArgv.emplace_back("");  // First argument is an executable
    llvm::cl::TokenizeGNUCommandLine(llvmOptionsStr, stringSaver, parsedArgv);
    llvm::cl::ParseCommandLineOptions(parsedArgv.size(), parsedArgv.data());
}

template <typename T>
void LLVMCompiler::SetLLVMOption(const char *option, T val)
{
    auto opts = llvm::cl::getRegisteredOptions();
    auto entry = opts.find(option);
    ASSERT(entry != opts.end());
    static_cast<llvm::cl::opt<T> *>(entry->second)->setValue(val);
}

// Instantiate for irtoc_compiler.cpp
template void LLVMCompiler::SetLLVMOption(const char *option, bool val);
// Instantiate for aot_compiler.cpp
template void LLVMCompiler::SetLLVMOption(const char *option, unsigned int val);
template void LLVMCompiler::SetLLVMOption(const char *option, uint64_t val);

llvm::Triple LLVMCompiler::GetTripleForArch(Arch arch)
{
    std::string error;
    std::string tripleName;
    switch (arch) {
        case Arch::AARCH64:
            tripleName = g_options.GetLlvmTriple();
            break;
        case Arch::X86_64:
#ifdef PANDA_TARGET_LINUX
            tripleName = g_options.WasSetLlvmTriple() ? g_options.GetLlvmTriple() : "x86_64-unknown-linux-gnu";
#elif defined(PANDA_TARGET_MACOS)
            tripleName = g_options.WasSetLlvmTriple() ? g_options.GetLlvmTriple() : "x86_64-apple-darwin-gnu";
#elif defined(PANDA_TARGET_WINDOWS)
            tripleName = g_options.WasSetLlvmTriple() ? g_options.GetLlvmTriple() : "x86_64-unknown-windows-unknown";
#else
            tripleName = g_options.WasSetLlvmTriple() ? g_options.GetLlvmTriple() : "x86_64-unknown-unknown-unknown";
#endif
            break;

        default:
            UNREACHABLE();
    }
    llvm::Triple triple(llvm::Triple::normalize(tripleName));
    [[maybe_unused]] auto target = llvm::TargetRegistry::lookupTarget("", triple, error);
    ASSERT_PRINT(target != nullptr, error);
    return triple;
}

std::string LLVMCompiler::GetCPUForArch(Arch arch)
{
    std::string cpu;
    switch (arch) {
        case Arch::AARCH64:
            cpu = g_options.GetLlvmCpu();
            break;
        case Arch::X86_64:
            cpu = g_options.WasSetLlvmCpu() ? g_options.GetLlvmCpu() : "";
            break;
        default:
            UNREACHABLE();
    }
    return cpu;
}

void LLVMCompiler::InitializeLLVMTarget(Arch arch)
{
    switch (arch) {
        case Arch::AARCH64: {
            LLVMInitializeAArch64TargetInfo();
            LLVMInitializeAArch64Target();
            LLVMInitializeAArch64TargetMC();
            LLVMInitializeAArch64AsmPrinter();
            LLVMInitializeAArch64AsmParser();
            break;
        }
#ifdef PANDA_TARGET_AMD64
        case Arch::X86_64: {
            LLVMInitializeX86TargetInfo();
            LLVMInitializeX86Target();
            LLVMInitializeX86TargetMC();
            LLVMInitializeX86AsmPrinter();
            LLVMInitializeX86AsmParser();
            break;
        }
#endif
        default:
            LLVM_LOG(FATAL, INFRA) << GetArchString(arch) << std::string(" is unsupported. ");
    }
    LLVM_LOG(DEBUG, INFRA) << "Available targets";
    for (auto target : llvm::TargetRegistry::targets()) {
        LLVM_LOG(DEBUG, INFRA) << "\t" << target.getName();
    }
}

void LLVMCompiler::InitializeLLVMPasses()
{
    llvm::PassRegistry &registry = *llvm::PassRegistry::getPassRegistry();
    initializeCore(registry);
    initializeScalarOpts(registry);
    initializeObjCARCOpts(registry);
    initializeVectorization(registry);
    initializeIPO(registry);
    initializeAnalysis(registry);
    initializeTransformUtils(registry);
    initializeInstCombine(registry);
    initializeAggressiveInstCombine(registry);
    initializeInstrumentation(registry);
    initializeTarget(registry);
    initializeCodeGen(registry);
    initializeGlobalISel(registry);
}

}  // namespace panda::llvmbackend
