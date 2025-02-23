use std::fs;

fn main() -> Result<(), &'static str> {
    for i in fs::read_dir("src").unwrap() {
        println!("cargo:rerun-if-changed={}", i.unwrap().path().display());
    }

    cxx_build::bridge("src/lib.rs").flag_if_supported("-std=c++23").compile("scx-rustlib");

    Ok(())
}
