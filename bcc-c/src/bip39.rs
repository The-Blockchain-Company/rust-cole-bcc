use std::slice;

use bcc::bip::bip39;
use types::BccBIP39ErrorCode;
use types::BccResult;

use std::{
    os::raw::{c_char, c_uchar, c_uint},
    ptr,
};

use std::ffi::CStr;

/// encode a entropy into its equivalent words represented by their index (0 to 2047) in the BIP39 dictionary
#[no_mangle]
pub extern "C" fn bcc_bip39_encode(
    entropy_raw: *const u8,             /* raw entropy */
    entropy_bytes: usize,               /* the number of bytes to encode */
    encoded: *mut bip39::MnemonicIndex, /* the encoded entropy */
    encoded_size: usize,
) -> BccResult {
    let in_slice = unsafe { slice::from_raw_parts(entropy_raw, entropy_bytes) };
    let out_slice = unsafe { slice::from_raw_parts_mut(encoded, encoded_size) };
    let entropy = match bip39::Entropy::from_slice(in_slice) {
        Ok(e) => e,
        Err(_) => return BccResult::failure(),
    };
    out_slice.copy_from_slice(entropy.to_mnemonics().as_ref());
    BccResult::success()
}

///retrieve the entropy from the given english mnemonics
#[no_mangle]
pub extern "C" fn bcc_entropy_from_english_mnemonics(
    mnemonics: *const c_char,
    entropy_ptr: *mut *const c_uchar,
    entropy_size: *mut c_uint,
) -> BccBIP39ErrorCode {
    let rust_string = unsafe { CStr::from_ptr(mnemonics) }.to_string_lossy();

    let dictionary = bip39::dictionary::ENGLISH;

    let mnemonics = match bip39::Mnemonics::from_string(&dictionary, &rust_string) {
        Ok(m) => m,
        //The error happens when a word is not in the dictionary
        Err(_) => return BccBIP39ErrorCode::invalid_word(),
    };

    let entropy = match bip39::Entropy::from_mnemonics(&mnemonics) {
        Ok(e) => e,
        //The error happens because the phrase doesn't have a valid checksum
        Err(_) => return BccBIP39ErrorCode::invalid_checksum(),
    };

    out_return_vector(entropy.to_vec(), entropy_ptr, entropy_size);

    BccBIP39ErrorCode::success()
}

///generate entropy from the given random generator
#[no_mangle]
pub extern "C" fn bcc_entropy_from_random(
    words: u8,
    gen: extern "C" fn() -> c_uchar,
    entropy_ptr: *mut *const c_uchar,
    entropy_size: *mut c_uint,
) -> BccBIP39ErrorCode {
    let words = match bip39::Type::from_word_count(words as usize) {
        Ok(v) => v,
        Err(_) => return BccBIP39ErrorCode::invalid_word_count(),
    };

    let entropy = bip39::Entropy::generate(words, || gen()).to_vec();

    out_return_vector(entropy, entropy_ptr, entropy_size);

    BccBIP39ErrorCode::success()
}

///return C array as an out parameter, the memory must be then deallocated with bcc_delete_entropy_array
fn out_return_vector(mut to_return: Vec<u8>, out_pointer: *mut *const c_uchar, size: *mut c_uint) {
    //Make sure the capacity is the same as the length to make deallocation simpler
    to_return.shrink_to_fit();

    let pointer = to_return.as_mut_ptr();
    let length = to_return.len() as u32;

    //To avoid running the destructor
    std::mem::forget(to_return);

    //Write the array length
    unsafe { ptr::write(size, length) }

    //Copy the pointer to the out parameter
    unsafe { ptr::write(out_pointer, pointer) };
}

//Deallocate the rust-allocated memory for a Entropy array
#[no_mangle]
pub extern "C" fn bcc_delete_entropy_array(ptr: *mut c_uchar, size: u32) {
    let len = size as usize;
    unsafe { drop(Vec::from_raw_parts(ptr, len, len)) };
}
