// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2024-2025 Vladislav Nepogodin <vnepogodin@cachyos.org>

// This software may be used and distributed according to the terms of the
// GNU General Public License version 2.

use std::process::{Command, Stdio};

pub fn exec(command: &str) -> String {
    let output = Command::new("sh")
        .arg("-c")
        .arg(command)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .output();

    if let Ok(output) = output {
        if !output.status.success() {
            return "-1".to_string();
        }

        let mut result = String::from_utf8_lossy(&output.stdout).to_string();
        if result.ends_with('\n') {
            result.pop();
        }

        result
    } else {
        "-1".to_string()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_exec_success() {
        let result = exec("echo hello");
        assert_eq!(result, "hello");
    }

    #[test]
    fn test_exec_fail() {
        let result = exec("false"); // false command always fails
        assert_eq!(result, "-1");
    }

    #[test]
    fn test_exec_with_newline() {
        let result = exec("printf '%s\\n%s' hello world");
        assert_eq!(result, "hello\nworld"); // Check that newline is preserved within the output
    }

    #[test]
    fn test_exec_trailing_newline() {
        let result = exec("echo hello");
        assert!(!result.ends_with('\n')); // Ensure trailing newline is removed
    }

    #[test]
    fn test_exec_empty_command() {
        let result = exec("");
        assert!(!result.ends_with('\n'));
    }

    #[test]
    fn test_exec_complex_command() {
        // A more complex command demonstrating piping
        let result = exec("echo 'line1\nline2' | grep line1");
        assert_eq!(result, "line1");
    }

    #[test]
    fn test_exec_stderr_output() {
        // Redirect stderr to stdout
        let result = exec("ls /nonexistent/directory 2>&1");
        assert_eq!(result, "-1");
    }
}
