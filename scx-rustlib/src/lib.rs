// Copyright (C) 2024-2025 Vladislav Nepogodin
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

pub mod utils;

use scx_loader::dbus::LoaderClientProxy;
use scx_loader::*;

use std::fs;
use std::io::Write;
use std::path::Path;
use std::process::Command;

use anyhow::{Context, Result};
use tokio::runtime::Runtime;
use zbus::Connection;

#[cxx::bridge(namespace = "scx_loader")]
mod ffi {
    extern "Rust" {
        type Config;

        /// Initialize config from config path, if the file doesn't exist config with default values
        /// will be created.
        fn init_config_file(config_path: &str) -> Result<Box<Config>>;

        /// Get the scx flags for the given sched mode
        fn get_scx_flags_for_mode(
            &self,
            supported_sched: &str,
            sched_mode: u32,
        ) -> Result<Vec<String>>;

        /// Applies the scx scheduler with arguments/mode
        fn apply_scheduler_change(
            &mut self,
            scx_name: &str,
            scx_mode: u32,
            extra_flags: &str,
            config_path: &str,
        ) -> Result<()>;

        /// Disables auto start of scheduler, and stops current scheduler
        fn disable_scheduler(&mut self, config_path: &str) -> Result<()>;

        /// Returns a list of the schedulers currently supported by the Scheduler Loader.
        fn get_supported_scheds() -> Result<Vec<String>>;

        /// Returns currently running scheduler.
        fn get_current_sched(&self) -> Result<String>;

        /// Returns currently running scheduler mode.
        fn get_current_mode(&self) -> Result<u8>;
    }
}

pub struct Config {
    config: scx_loader::config::Config,
}

fn init_config_file(config_path: &str) -> Result<Box<Config>> {
    let config = init_config(config_path).context("Failed to initialize config")?;
    Ok(Box::new(Config { config }))
}

impl Config {
    /// Write the config to the file.
    fn write_config_file(&self, filepath: &str) -> Result<()> {
        let toml_content = toml::to_string(&self.config)?;

        let mut file_obj = fs::File::create(filepath)?;
        file_obj.write_all(toml_content.as_bytes())?;

        Ok(())
    }

    fn get_scx_flags_for_mode(
        &self,
        supported_sched: &str,
        sched_mode: u32,
    ) -> Result<Vec<String>> {
        let scx_sched = get_scx_from_str(supported_sched)?;
        let sched_mode = convert_from_raw_mode(sched_mode)?;
        let args = config::get_scx_flags_for_mode(&self.config, &scx_sched, sched_mode);
        Ok(args)
    }

    /// Set the default scheduler with default mode
    fn set_scx_sched_with_mode(&mut self, scx_sched: SupportedSched, sched_mode: SchedMode) {
        self.config.default_sched = Some(scx_sched);
        self.config.default_mode = Some(sched_mode);
    }

    /// Set args for the scheduler with mode
    fn set_scx_args(
        &mut self,
        scx_sched: SupportedSched,
        sched_mode: SchedMode,
        sched_args: Vec<String>,
    ) {
        let scx_name: &str = scx_sched.into();
        let sched = self.config.scheds.entry(scx_name.into()).or_default();

        let args_to_set = match sched_mode {
            SchedMode::Gaming => &mut sched.gaming_mode,
            SchedMode::LowLatency => &mut sched.lowlatency_mode,
            SchedMode::PowerSave => &mut sched.powersave_mode,
            SchedMode::Server => &mut sched.server_mode,
            SchedMode::Auto => &mut sched.auto_mode,
        };

        *args_to_set = Some(sched_args);
    }

    /// Disables auto start of scheduler, and stops current scheduler
    fn disable_scx_sched(&mut self, config_path: &str) -> Result<()> {
        // for us to correctly disable it:
        // 1. set `default_sched` to None
        // 2. edit config file
        // 3. call method to stop current scheduler

        // 1.
        self.config.default_sched = None;

        // 2.
        self.write_config_file(config_path).context("Failed to edit config file")?;

        // 3.
        let rt = Runtime::new().context("Failed to initialize tokio runtime")?;
        rt.block_on(async move {
            let connection = Connection::system().await?;
            let loader_client = LoaderClientProxy::new(&connection).await?;
            loader_client.stop_scheduler().await?;

            anyhow::Ok(())
        })?;

        Ok(())
    }

    fn switch_scheduler_with_args(
        &self,
        scx_sched: SupportedSched,
        scx_args: &[String],
    ) -> Result<()> {
        let rt = Runtime::new().context("Failed to initialize tokio runtime")?;
        rt.block_on(async move {
            let connection = Connection::system().await?;
            let loader_client = LoaderClientProxy::new(&connection).await?;
            loader_client.switch_scheduler_with_args(scx_sched, scx_args).await?;

            anyhow::Ok(())
        })?;

        Ok(())
    }

    fn switch_scheduler_with_mode(
        &self,
        scx_sched: SupportedSched,
        scx_mode: SchedMode,
    ) -> Result<()> {
        let rt = Runtime::new().context("Failed to initialize tokio runtime")?;
        rt.block_on(async move {
            let connection = Connection::system().await?;
            let loader_client = LoaderClientProxy::new(&connection).await?;
            loader_client.switch_scheduler(scx_sched, scx_mode).await?;

            anyhow::Ok(())
        })?;

        Ok(())
    }

    fn apply_scheduler_change(
        &mut self,
        scx_name: &str,
        scx_mode: u32,
        extra_flags: &str,
        config_path: &str,
    ) -> Result<()> {
        // stop/disable 'scx.service' if its running/enabled on the system,
        // otherwise it will conflict
        disable_scx_service();

        let default_args = self.get_scx_flags_for_mode(&scx_name, scx_mode)?;
        let scx_sched = get_scx_from_str(scx_name)?;
        let scx_mode = convert_from_raw_mode(scx_mode)?;

        let mut sched_args: Vec<String> = vec![];
        if !extra_flags.is_empty() {
            // NOTE: should be Vec instead of str
            let mut extra_args: Vec<_> = extra_flags.split(' ').map(String::from).collect();
            sched_args.append(&mut extra_args);
        }

        if sched_args == default_args {
            println!("Applying scx '{scx_name}' with mode {scx_mode:?}");
            if let Err(scx_err) =
                self.switch_scheduler_with_mode(scx_sched.clone(), scx_mode.clone())
            {
                println!("Failed to switch '{scx_name}' with mode {scx_mode:?}: {scx_err}");
            }
        } else {
            println!("Applying scx '{scx_name}' with args: {}", sched_args.join(" ").to_owned());
            if let Err(scx_err) = self.switch_scheduler_with_args(scx_sched.clone(), &sched_args) {
                println!("Failed to switch '{scx_name}' with args: {sched_args:?}: {scx_err}");
            }
        }

        // enable scx_loader service if not enabled yet, it fully replaces scx.service
        if !is_scx_loader_service_enabled() {
            println!("Enabling scx_loader service");
            spawn_child_process("/usr/bin/systemctl", &["enable", "-f", "scx_loader"]);
        }

        // change default scheduler and default scheduler mode
        self.set_scx_sched_with_mode(scx_sched.clone(), scx_mode.clone());

        // change args for the scheduler with mode
        // only if sched is not default for the mode
        if sched_args != default_args {
            self.set_scx_args(scx_sched, scx_mode, sched_args);
        }

        // write scx_loader configuration to the temp file
        let tmp_config_path = "/tmp/scx_loader.toml";
        self.write_config_file(tmp_config_path)
            .context("Cannot write scx_loader config to file")?;

        // copy scx_loader configuration from the temp file to the actual path with root permissions
        spawn_child_process("/usr/bin/pkexec", &["/usr/bin/cp", tmp_config_path, config_path]);

        Ok(())
    }

    fn disable_scheduler(&mut self, config_path: &str) -> Result<()> {
        // write scx_loader configuration to the temp file
        let tmp_config_path = "/tmp/scx_loader.toml";
        self.disable_scx_sched(tmp_config_path).context("Cannot disable scx_loader")?;

        // copy scx_loader configuration from the temp file to the actual path with root permissions
        spawn_child_process("/usr/bin/pkexec", &["/usr/bin/cp", tmp_config_path, config_path]);

        Ok(())
    }

    fn get_current_sched(&self) -> Result<String> {
        let rt = Runtime::new().context("Failed to initialize tokio runtime")?;
        let current_sched = rt.block_on(async move {
            let connection = Connection::system().await?;
            let loader_client = LoaderClientProxy::new(&connection).await?;
            let current_sched = loader_client.current_scheduler().await?;

            anyhow::Ok(current_sched)
        })?;

        Ok(current_sched)
    }

    fn get_current_mode(&self) -> Result<u8> {
        let rt = Runtime::new().context("Failed to initialize tokio runtime")?;
        let current_mode = rt.block_on(async move {
            let connection = Connection::system().await?;
            let loader_client = LoaderClientProxy::new(&connection).await?;
            let current_mode = loader_client.scheduler_mode().await?;

            anyhow::Ok(current_mode as u8)
        })?;

        Ok(current_mode)
    }
}

pub fn get_supported_scheds() -> Result<Vec<String>> {
    let rt = Runtime::new().context("Failed to initialize tokio runtime")?;
    return rt.block_on(async move {
        let connection = Connection::system().await?;
        let loader_client = LoaderClientProxy::new(&connection).await?;
        anyhow::Ok(loader_client.supported_schedulers().await?)
    });
}

/// Initialize config from first found config path, otherwise fallback to default config
pub fn init_config(config_path: &str) -> Result<scx_loader::config::Config> {
    if Path::new(config_path).exists() {
        scx_loader::config::parse_config_file(config_path)
    } else {
        Ok(scx_loader::config::get_default_config())
    }
}

/// Get the scx trait from the given scx name or return error if the given scx name is not supported
fn get_scx_from_str(scx_name: &str) -> Result<SupportedSched> {
    scx_name.parse()
}

fn convert_from_raw_mode(raw_mode: u32) -> Result<SchedMode> {
    match raw_mode {
        0 => Ok(SchedMode::Auto),
        1 => Ok(SchedMode::Gaming),
        2 => Ok(SchedMode::PowerSave),
        3 => Ok(SchedMode::LowLatency),
        4 => Ok(SchedMode::Server),
        _ => anyhow::bail!("SchedMode with such value doesn't exist"),
    }
}

fn spawn_child_process(cmd: &str, args: &[&str]) {
    let status = Command::new(cmd).args(args).status().expect("failed to execute child process");

    if !status.success() {
        println!("child process failed with exit code: {status:?}");
    }
}

fn is_scx_loader_service_enabled() -> bool {
    return utils::exec("systemctl is-enabled scx_loader") == "enabled";
}

fn is_scx_service_enabled() -> bool {
    return utils::exec("systemctl is-enabled scx") == "enabled";
}

fn is_scx_service_active() -> bool {
    return utils::exec("systemctl is-active scx") == "active";
}

fn disable_scx_service() {
    if is_scx_service_enabled() {
        spawn_child_process("/usr/bin/systemctl", &["disable", "--now", "-f", "scx"]);
        println!("Disabling scx service");
    } else if is_scx_service_active() {
        spawn_child_process("/usr/bin/systemctl", &["stop", "-f", "scx"]);
        println!("Stopping scx service");
    }
}
