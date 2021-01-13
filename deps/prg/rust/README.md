https://www.rust-lang.org/zh-CN/learn/get-started
https://github.com/rust-unofficial/awesome-rust
# error refs : https://www.jianshu.com/p/fa3b8b4896cc
Install:
#curl https://sh.rustup.rs -sSf | sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
 touch main.rs
 rustc main.rs
 mkdir -v src
 mv main.rs src
 vim Cargo.toml
cargo build --bin=appsrc
 cargo build --release
 cargo run
cargo check
cargo doc --open

rustup update
