# error refs : https://www.jianshu.com/p/fa3b8b4896cc
Install:
curl https://sh.rustup.rs -sSf | sh
 touch main.rs
 rustc main.rs
 mkdir -v src
 mv main.rs src
 vim Cargo.toml
cargo build --bin=appsrc
 cargo build --release
 cargo run
