// Copyright (C) 2022-2025 Vladislav Nepogodin
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

#ifndef SCHEDEXT_WINDOW_HPP_
#define SCHEDEXT_WINDOW_HPP_

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#pragma clang diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#pragma GCC diagnostic ignored "-Wsuggest-final-methods"
#pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"
#endif

#include <QtCore/QtGlobal>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#if defined(SCHEDEXT_LIB)
#define SCHEDEXT_EXPORT Q_DECL_EXPORT
#else
#define SCHEDEXT_EXPORT Q_DECL_IMPORT
#endif

class QWidget;

namespace scxctl {

namespace impl {
    class SchedExtWindow;
}  // namespace impl

class SCHEDEXT_EXPORT SchedExtWindow final {
 public:
    explicit SchedExtWindow(QWidget* parent = nullptr);
    ~SchedExtWindow();

    // very minimal and basic exposed func
    void show() noexcept;
    void hide() noexcept;
    bool isVisible() const noexcept;
    void setParent(QWidget* parent) noexcept;

 private:
    impl::SchedExtWindow* m_impl = nullptr;
};

SCHEDEXT_EXPORT auto create_schedext_window(QWidget* parent = nullptr) noexcept -> SchedExtWindow;

}  // namespace scxctl

#endif  // SCHEDEXT_WINDOW_HPP_
