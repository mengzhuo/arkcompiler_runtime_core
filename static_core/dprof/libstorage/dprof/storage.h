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

#ifndef DPROF_LIBSTORAGE_DPROF_STORAGE_H
#define DPROF_LIBSTORAGE_DPROF_STORAGE_H

#include "macros.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace panda::dprof {
class AppData {
public:
    using FeaturesMap = std::unordered_map<std::string, std::vector<uint8_t>>;
    ~AppData() = default;

    static std::unique_ptr<AppData> CreateByParams(const std::string &name, uint64_t hash, uint32_t pid,
                                                   FeaturesMap &&features_map);
    static std::unique_ptr<AppData> CreateByBuffer(const std::vector<uint8_t> &buffer);

    bool ToBuffer(std::vector<uint8_t> &out_buffer) const;

    std::string GetName() const
    {
        return common_info_.name;
    }

    uint64_t GetHash() const
    {
        return common_info_.hash;
    }

    uint32_t GetPid() const
    {
        return common_info_.pid;
    }

    const FeaturesMap &GetFeaturesMap() const
    {
        return features_map_;
    }

private:
    AppData() = default;

    NO_COPY_SEMANTIC(AppData);
    NO_MOVE_SEMANTIC(AppData);

    struct CommonInfo {
        std::string name {};
        uint64_t hash {};
        uint32_t pid {};
    };

    CommonInfo common_info_;
    FeaturesMap features_map_;
};

class AppDataStorage {
public:
    ~AppDataStorage() = default;

    static const size_t MAX_BUFFER_SIZE = 16 * 1024 * 1024;  // 16MB

    static std::unique_ptr<AppDataStorage> Create(const std::string &storage_dir, bool create_dir = false);

    bool SaveAppData(const AppData &app_data);
    void ForEachApps(const std::function<bool(std::unique_ptr<AppData> &&)> &callback) const;

private:
    explicit AppDataStorage(std::string storage_dir) : storage_dir_(std::move(storage_dir)) {}

    NO_COPY_SEMANTIC(AppDataStorage);
    NO_MOVE_SEMANTIC(AppDataStorage);

    std::string MakeAppPath(const std::string &name, uint64_t hash, uint32_t pid) const;

    std::string storage_dir_;
};
}  // namespace panda::dprof

#endif  // DPROF_LIBSTORAGE_DPROF_STORAGE_H
