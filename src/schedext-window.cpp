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

// NOLINTBEGIN(bugprone-unhandled-exception-at-new)

#include "schedext-window.hpp"
#include "schedext-window-internal.hpp"

namespace scxctl {

SchedExtWindow::SchedExtWindow(QWidget* parent)
  : m_impl(new impl::SchedExtWindow(parent)) {
}

SchedExtWindow::~SchedExtWindow() {
    delete m_impl;
    m_impl = nullptr;
}

void SchedExtWindow::show() noexcept {
    m_impl->show();
}

void SchedExtWindow::hide() noexcept {
    m_impl->hide();
}

bool SchedExtWindow::isVisible() const noexcept {
    return m_impl->isVisible();
}

void SchedExtWindow::setParent(QWidget* parent) noexcept {
    m_impl->setParent(parent);
}

auto create_schedext_window(QWidget* parent) noexcept -> SchedExtWindow {
    return SchedExtWindow(parent);
}

}  // namespace scxctl

// NOLINTEND(bugprone-unhandled-exception-at-new)
