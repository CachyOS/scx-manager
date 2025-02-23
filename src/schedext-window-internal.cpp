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

#include "schedext-window-internal.hpp"
#include "schedext-window.hpp"
#include "scx_utils.hpp"

#include <fstream>
#include <string>
#include <string_view>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsuggest-final-types"
#pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"
#pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <QMessageBox>
#include <QProcess>
#include <QStringList>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <fmt/compile.h>
#include <fmt/core.h>

namespace {
auto read_kernel_file(std::string_view file_path) noexcept -> std::string {
    std::string file_content{};

    // Skip if failed to open file descriptor.
    std::ifstream file_stream{std::string{file_path}};
    if (!file_stream.is_open()) {
        return {};
    }
    if (!std::getline(file_stream, file_content)) {
        fmt::print(stderr, "Failed to read := '{}'\n", file_path);
        return {};
    }

    return file_content;
}

auto get_current_scheduler() noexcept -> std::string {
    using namespace std::string_view_literals;

    // NOTE: we assume that window won't be launched on kernel without sched_ext
    // e.g we won't show window at all in that case.
    const auto& current_state = read_kernel_file("/sys/kernel/sched_ext/state"sv);
    if (current_state != "enabled"sv) {
        return current_state;
    }
    const auto& current_sched = read_kernel_file("/sys/kernel/sched_ext/root/ops"sv);
    if (current_sched.empty()) {
        return std::string{"unknown"sv};
    }
    return current_sched;
}

constexpr auto get_scx_mode_from_str(std::string_view scx_mode) noexcept -> scx::SchedMode {
    using namespace std::string_view_literals;

    if (scx_mode == "Gaming"sv) {
        return scx::SchedMode::Gaming;
    } else if (scx_mode == "Lowlatency"sv) {
        return scx::SchedMode::LowLatency;
    } else if (scx_mode == "Powersave"sv) {
        return scx::SchedMode::PowerSave;
    } else if (scx_mode == "Server"sv) {
        return scx::SchedMode::Server;
    }
    return scx::SchedMode::Auto;
}

}  // namespace

namespace scxctl::impl {

SchedExtWindow::SchedExtWindow(QWidget* parent)
  : QMainWindow(parent), m_sched_timer(new QTimer(this)) {
    m_ui->setupUi(this);

    setAttribute(Qt::WA_NativeWindow);
    setWindowFlags(Qt::Window);  // for the close, min and max buttons

    {
        auto loader_config = scx::loader::Config::init_config(m_config_path);
        if (loader_config.has_value()) {
            m_scx_config = std::make_unique<scx::loader::Config>(std::move(*loader_config));
        } else {
            QMessageBox::critical(this, "CachyOS Kernel Manager", tr("Cannot initialize scx_loader configuration"));
            return;
        }
    }

    // Timer updates information about currently running scheduler even without scx_loader,
    // as it reads information reported by scx scheduler.
    using namespace std::chrono_literals;  // NOLINT
    connect(m_sched_timer, &QTimer::timeout, this, &SchedExtWindow::update_current_sched);
    m_sched_timer->start(1s);

    // Selecting the scheduler
    auto supported_scheds = scx::loader::get_supported_scheds();
    if (supported_scheds.has_value()) {
        m_ui->schedext_combo_box->addItems(*supported_scheds);
    } else {
        QMessageBox::critical(this, "CachyOS Kernel Manager", tr("Cannot get information from scx_loader!\nIs it working?\nThis is needed for the app to work properly"));

        // hide all components which depends on scheduler management
        m_ui->schedext_combo_box->setHidden(true);
        m_ui->scheduler_select_label->setHidden(true);

        m_ui->schedext_profile_combo_box->setHidden(true);
        m_ui->scheduler_profile_select_label->setHidden(true);

        m_ui->schedext_flags_edit->setHidden(true);
        m_ui->scheduler_set_flags_label->setHidden(true);
        return;
    }

    // Set currently running scheduler
    auto current_sched = m_scx_config->get_current_sched();
    if (current_sched.has_value()) {
        m_ui->schedext_combo_box->setCurrentText(QString::fromStdString(*current_sched));
    }

    // Selecting the performance profile
    QStringList sched_profiles;
    sched_profiles << "Auto"
                   << "Gaming"
                   << "Powersave"
                   << "Lowlatency"
                   << "Server";
    m_ui->schedext_profile_combo_box->addItems(sched_profiles);
    connect(m_ui->schedext_profile_combo_box,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        &SchedExtWindow::on_sched_profile_changed);

    // Set currently runing scheduler mode
    auto current_mode = m_scx_config->get_current_mode();
    if (current_mode.has_value()) {
        // NOTE: the index of profiles and scxmode values MUST match
        m_ui->schedext_profile_combo_box->setCurrentIndex(static_cast<std::uint8_t>(*current_mode));
    }

    m_ui->current_sched_label->setText(QString::fromStdString(get_current_scheduler()));

    connect(m_ui->schedext_combo_box,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        &SchedExtWindow::on_sched_changed);
    // Initialize the visibility of the profile selection box
    on_sched_changed();

    // Connect buttons signal
    connect(m_ui->apply_button, &QPushButton::clicked, this, &SchedExtWindow::on_apply);
    connect(m_ui->disable_button, &QPushButton::clicked, this, &SchedExtWindow::on_disable);
}

void SchedExtWindow::closeEvent(QCloseEvent* event) {
    QWidget::closeEvent(event);
}

void SchedExtWindow::update_current_sched() noexcept {
    m_ui->current_sched_label->setText(QString::fromStdString(get_current_scheduler()));
}

void SchedExtWindow::on_disable() noexcept {
    m_ui->disable_button->setEnabled(false);
    m_ui->apply_button->setEnabled(false);

    if (!m_scx_config->disable_scheduler(m_config_path)) {
        QMessageBox::critical(this, "CachyOS Kernel Manager", tr("Cannot disable scx_loader"));
    }

    m_ui->disable_button->setEnabled(true);
    m_ui->apply_button->setEnabled(true);
}

void SchedExtWindow::on_sched_profile_changed() noexcept {
    const auto& current_selected = m_ui->schedext_combo_box->currentText().toStdString();
    const auto& current_profile  = m_ui->schedext_profile_combo_box->currentText().toStdString();
    const auto& scx_mode         = get_scx_mode_from_str(current_profile);

    auto sched_args = QStringList();
    if (auto scx_flags_for_mode = m_scx_config->scx_flags_for_mode(current_selected, scx_mode); scx_flags_for_mode) {
        if (!scx_flags_for_mode->empty()) {
            sched_args << std::move(*scx_flags_for_mode);
        }
    } else {
        QMessageBox::critical(this, "CachyOS Kernel Manager", tr("Cannot get scx flags from scx_loader configuration!"));
    }

    m_ui->schedext_flags_edit->setText(sched_args.join(' '));
}

void SchedExtWindow::on_sched_changed() noexcept {
    const auto& scheduler = m_ui->schedext_combo_box->currentText();

    // Show or hide the profile selection UI based on the selected scheduler
    //
    // NOTE: only scx_bpfland and scx_lavd support different preset profiles at
    // the moment.
    if (scheduler == "scx_bpfland" || scheduler == "scx_lavd") {
        m_ui->scheduler_profile_select_label->setVisible(true);
        m_ui->schedext_profile_combo_box->setVisible(true);
    } else {
        m_ui->scheduler_profile_select_label->setVisible(false);
        m_ui->schedext_profile_combo_box->setVisible(false);
    }
    on_sched_profile_changed();
}

void SchedExtWindow::on_apply() noexcept {
    m_ui->disable_button->setEnabled(false);
    m_ui->apply_button->setEnabled(false);

    // TODO(vnepogodin): refactor that
    const auto& current_selected = m_ui->schedext_combo_box->currentText().toStdString();
    const auto& current_profile  = m_ui->schedext_profile_combo_box->currentText().toStdString();
    const auto& extra_flags      = m_ui->schedext_flags_edit->text().trimmed().toStdString();
    const auto& scx_mode         = get_scx_mode_from_str(current_profile);

    if (!m_scx_config->apply_scheduler_change(current_selected, scx_mode, extra_flags, m_config_path)) {
        QMessageBox::critical(this, "CachyOS Kernel Manager", tr("Cannot set default scx scheduler with mode! Scheduler %1 with mode %2").arg(QString::fromStdString(current_selected), QString::fromStdString(current_profile)));
    }

    m_ui->disable_button->setEnabled(true);
    m_ui->apply_button->setEnabled(true);
}

}  // namespace scxctl::impl

// NOLINTEND(bugprone-unhandled-exception-at-new)
