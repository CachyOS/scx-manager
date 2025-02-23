// Copyright (C) 2024-2025 Vladislav Nepogodin
//
// This file is part of CachyOS kernel manager.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include "scx_utils.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wdeprecated-this-capture"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsuggest-final-types"
#pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include "scx-lib-cxxbridge/lib.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <fmt/core.h>

namespace {

auto convert_rust_vec_string(auto&& rust_vec) -> std::vector<std::string> {
    std::vector<std::string> res_vec{};
    res_vec.reserve(rust_vec.size());
    for (auto&& rust_vec_el : rust_vec) {
        res_vec.emplace_back(std::string{rust_vec_el});
    }
    return res_vec;
}

auto convert_std_vec_into_stringlist(std::vector<std::string>&& std_vec) -> QStringList {
    QStringList flags;
    flags.reserve(static_cast<qsizetype>(std_vec.size()));
    for (auto&& vec_el : std_vec) {
        flags << QString::fromStdString(vec_el);
    }
    return flags;
}

auto get_scx_mode_from_raw(std::uint8_t raw_mode) noexcept -> scx::SchedMode {
    switch (raw_mode) {
    case 0:
        return scx::SchedMode::Auto;
    case 1:
        return scx::SchedMode::Gaming;
    case 2:
        return scx::SchedMode::PowerSave;
    case 3:
        return scx::SchedMode::LowLatency;
    case 4:
        return scx::SchedMode::Server;
    default:
        fmt::print(stderr, "SchedMode with such value doesn't exist: {}\n", raw_mode);
        break;
    }
    return scx::SchedMode::Auto;
}

}  // namespace

namespace scx::loader {

auto get_supported_scheds() noexcept -> std::optional<QStringList> {
    try {
        auto rust_vec  = ::scx_loader::get_supported_scheds();
        auto flags_vec = convert_rust_vec_string(std::move(rust_vec));
        return convert_std_vec_into_stringlist(std::move(flags_vec));
    } catch (const std::exception& e) {
        fmt::print(stderr, "Failed to get supported schedulers: {}\n", e.what());
    }
    return std::nullopt;
}

auto Config::init_config(std::string_view filepath) noexcept -> std::optional<Config> {
    try {
        const ::rust::Str filepath_rust(filepath.data(), filepath.size());
        auto&& config = Config(scx_loader::init_config_file(filepath_rust));
        return std::make_optional<Config>(std::move(config));
    } catch (const std::exception& e) {
        fmt::print(stderr, "Failed to parse init config: {}\n", e.what());
    }
    return std::nullopt;
}

auto Config::scx_flags_for_mode(std::string_view scx_sched, SchedMode sched_mode) noexcept -> std::optional<QStringList> {
    try {
        const ::rust::Str scx_sched_rust(scx_sched.data(), scx_sched.size());
        auto rust_vec  = m_config->get_scx_flags_for_mode(scx_sched_rust, static_cast<std::uint32_t>(sched_mode));
        auto flags_vec = convert_rust_vec_string(std::move(rust_vec));
        return convert_std_vec_into_stringlist(std::move(flags_vec));
    } catch (const std::exception& e) {
        fmt::print(stderr, "Failed to get scx flag for the mode: {}\n", e.what());
    }
    return std::nullopt;
}

auto Config::apply_scheduler_change(std::string_view scx_sched, SchedMode sched_mode, std::string_view extra_flags, std::string_view filepath) noexcept -> bool {
    try {
        const ::rust::Str scx_sched_rust(scx_sched.data(), scx_sched.size());
        const ::rust::Str extra_flags_rust(extra_flags.data(), extra_flags.size());
        const ::rust::Str filepath_rust(filepath.data(), filepath.size());
        m_config->apply_scheduler_change(scx_sched_rust, static_cast<std::uint32_t>(sched_mode), extra_flags_rust, filepath_rust);
        return true;
    } catch (const std::exception& e) {
        fmt::print(stderr, "Failed to apply scx scheduler change: {}\n", e.what());
    }
    return false;
}

auto Config::disable_scheduler(std::string_view filepath) noexcept -> bool {
    try {
        const ::rust::Str filepath_rust(filepath.data(), filepath.size());
        m_config->disable_scheduler(filepath_rust);
        return true;
    } catch (const std::exception& e) {
        fmt::print(stderr, "Failed to disable scx scheduler: {}\n", e.what());
    }
    return false;
}

auto Config::get_current_sched() noexcept -> std::optional<std::string> {
    try {
        auto current_sched = m_config->get_current_sched();
        return std::string(current_sched);
    } catch (const std::exception& e) {
        fmt::print(stderr, "Failed to get currently running scx mode: {}\n", e.what());
    }
    return std::nullopt;
}

auto Config::get_current_mode() noexcept -> std::optional<SchedMode> {
    try {
        auto raw_mode = m_config->get_current_mode();
        return get_scx_mode_from_raw(raw_mode);
    } catch (const std::exception& e) {
        fmt::print(stderr, "Failed to get currently running scx mode: {}\n", e.what());
    }
    return std::nullopt;
}

}  // namespace scx::loader
