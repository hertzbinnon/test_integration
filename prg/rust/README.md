Install:
curl https://sh.rustup.rs -sSf | sh
 touch main.rs
 rustc main.rs
 mkdir -v src
 mv main.rs src
 vim Cargo.toml
 cargo build --release
 cargo run
