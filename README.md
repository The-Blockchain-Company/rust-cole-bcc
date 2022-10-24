:warning: This project is deprecated, and only supports the first version of bcc (Cole). do not use anymore, it is here for historical purpose.

# Rust implementation of Bcc primitives, helpers, and related applications

Bcc Rust is a modular toolbox of Bcc’s cryptographic primitives, a
library of wallet functions and a future alternative Bcc node
implementation written in Rust. It can be used by any third-party to build
wallet applications and interact with the Bcc blockchain.

## Related repositories

* [bcc-cli](https://github.com/the-blockchain-company/bcc-cli)

## Installation

If this is a new installation:
[install rust's toolchain](https://www.rust-lang.org/en-US/install.html).

We support the following states; `stable`, `unstable` and `nightly`.

We also support the `wasm32` target.

## Building the Library

To build the library, use:

```
cargo build
```

## Running the tests

To run the tests, use:

```
cargo test
```

## How to integrate the Rust library in your project

Information will be available soon on crates.io

In the mean time, it is possible to add the project using git submodules:

```git submodule add https://github.com/the-blockchain-company/rust-bcc bcc-deps```

And then by adding the following to your Cargo.toml:

```
[dependencies]
bcc = { path = "bcc-deps/bcc" }
```
