use bcc::address;
use bcc::block;
use bcc::coin::CoinDiff;
use bcc::hdwallet;
use bcc::tx;
use bcc::txbuild;
use bcc::wallet::bip44;
use std::os::raw::c_int;

/// C result type, where 0 is success and !0 is failure
#[repr(C)]
pub struct BccResult(c_int);

impl BccResult {
    pub fn success() -> BccResult {
        BccResult(0)
    }
    pub fn failure() -> BccResult {
        BccResult(1)
    }
}

///Struct for representing the possible BIP39 error codes
#[repr(C)]
pub struct BccBIP39ErrorCode(c_int);

impl BccBIP39ErrorCode {
    pub fn success() -> Self {
        BccBIP39ErrorCode(0)
    }

    ///Error representing a word not in the dictionary
    pub fn invalid_word() -> Self {
        BccBIP39ErrorCode(1)
    }

    ///Error representing that a mnemonic phrase checksum is incorrect
    pub fn invalid_checksum() -> Self {
        BccBIP39ErrorCode(2)
    }

    ///Error representing that the word count is not one of the supported ones
    pub fn invalid_word_count() -> Self {
        BccBIP39ErrorCode(3)
    }
}

#[repr(C)]
pub struct BccTransactionErrorCode(c_int);

impl BccTransactionErrorCode {
    pub fn success() -> Self {
        BccTransactionErrorCode(0)
    }

    ///Transaction has no outputs
    pub fn no_outputs() -> Self {
        BccTransactionErrorCode(1)
    }

    ///Transaction has no inputs
    pub fn no_inputs() -> Self {
        BccTransactionErrorCode(2)
    }

    ///Number of signatures does not match the number of witnesses
    pub fn signature_mismatch() -> Self {
        BccTransactionErrorCode(3)
    }

    ///Transaction is too big
    pub fn over_limit() -> Self {
        BccTransactionErrorCode(4)
    }

    ///Transaction has already enough signatures
    pub fn signatures_exceeded() -> Self {
        BccTransactionErrorCode(5)
    }

    ///value is to big, max = 45000000000000000
    pub fn coin_out_of_bounds() -> Self {
        BccTransactionErrorCode(6)
    }
}

impl From<txbuild::Error> for BccTransactionErrorCode {
    fn from(err: txbuild::Error) -> Self {
        match err {
            txbuild::Error::TxInvalidNoInput => Self::no_inputs(),
            txbuild::Error::TxInvalidNoOutput => Self::no_outputs(),
            txbuild::Error::TxNotEnoughTotalInput => unimplemented!(),
            txbuild::Error::TxOverLimit(_) => Self::over_limit(),
            txbuild::Error::TxOutputPolicyNotEnoughCoins(_) => unimplemented!(),
            txbuild::Error::TxSignaturesExceeded => Self::signatures_exceeded(),
            txbuild::Error::TxSignaturesMismatch => Self::signature_mismatch(),
            txbuild::Error::CoinError(_) => Self::coin_out_of_bounds(),
            txbuild::Error::FeeError(_) => unimplemented!(),
        }
    }
}

#[repr(C)]
pub enum DiffType {
    Positive,
    Negative,
    Zero,
}

#[repr(C)]
pub struct Balance {
    sign: DiffType,
    value: u64,
}

impl From<CoinDiff> for Balance {
    fn from(cd: CoinDiff) -> Self {
        match cd {
            CoinDiff::Positive(i) => Balance {
                sign: DiffType::Positive,
                value: i.into(),
            },
            CoinDiff::Negative(i) => Balance {
                sign: DiffType::Negative,
                value: i.into(),
            },
            CoinDiff::Zero => Balance {
                sign: DiffType::Zero,
                value: 0,
            },
        }
    }
}

/// C pointer to an Extended Private Key
pub type XPrvPtr = *mut hdwallet::XPrv;

/// C pointer to an Extended Public Key
pub type XPubPtr = *mut hdwallet::XPub;

/// C pointer to a signature
pub type SignaturePtr = *mut hdwallet::Signature<tx::Tx>;

/// C pointer to a (parsed) Extended Address
pub type AddressPtr = *mut address::ExtendedAddr;

/// C pointer to a Wallet
pub type WalletPtr = *mut bip44::Wallet;

/// C pointer to an Account;
pub type AccountPtr = *mut bip44::Account<hdwallet::XPub>;

/// C pointer to a Transaction output pointer;
pub type TransactionOutputPointerPtr = *mut tx::TxoPointer;

/// C pointer to a Transaction output;
pub type TransactionOutputPtr = *mut tx::TxOut;

/// C pointer to a Transaction;
pub type TransactionPtr = *mut tx::Tx;

/// C pointer to a signed Transaction;
pub type SignedTransactionPtr = *mut tx::TxAux;

/// C pointer to a Transaction builder;
pub type TransactionBuilderPtr = *mut txbuild::TxBuilder;

/// C pointer to a Transaction finalized;
pub type TransactionFinalizedPtr = *mut txbuild::TxFinalized;

/// C pointer to a Block;
pub type BlockPtr = *mut block::Block;

/// C pointer to a BlockHeader;
pub type BlockHeaderPtr = *mut block::BlockHeader;
