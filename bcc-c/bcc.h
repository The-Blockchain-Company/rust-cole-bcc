/*! \file bcc.h
*/
#ifndef BCC_RUST_H
#define BCC_RUST_H
/* Basic Types */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*!
* Type used to represent failure and success
*/
typedef enum _bcc_result {
    BCC_RESULT_SUCCESS = 0,
    BCC_RESULT_ERROR = 1
} bcc_result;

/*********/
/* BIP39 */
/*********/

/* bip39 error definitions */
typedef enum _bip39_config_error
{
    BIP39_SUCCESS = 0,
    BIP39_INVALID_MNEMONIC = 1,
    BIP39_INVALID_CHECKSUM = 2,
    BIP39_INVALID_WORD_COUNT = 3
} bcc_bip39_error_t;

typedef uint8_t* bcc_entropy;

/*!
* \brief get entropy array from the given english mnemonics 
* \param [in] mnemonics a string consisting of 9, 12, 15, 18, 21 or 24 english words
* \param [out] entropy the returned entropy array, use `bcc_delete_entropy_array` to release the memory
* \param [out] entropy_size the size of the the returned array
* \sa bcc_delete_entropy_array()
* \returns BIP39_SUCCESS or either BIP39_INVALID_MNEMONIC or BIP39_INVALID_CHECKSUM 
*/
bcc_bip39_error_t bcc_entropy_from_english_mnemonics(
    const char *mnemonics,
    bcc_entropy *entropy,
    uint32_t *entropy_size
);

/*!
* \brief encode a entropy into its equivalent words represented by their index (0 to 2047) in the BIP39 dictionary
* \param [in] number_of_words one of 9, 12, 15, 18, 21 or 24 representing the number of words of the equivalent mnemonic
* \param [in] random_generator a function that generates random bytes  
* \param [out] entropy the returned entropy array
* \param [out] entropy_size the size of the the returned array
* \returns BIP39_SUCCESS or BIP39_INVALID_WORD_COUNT 
*/
bcc_bip39_error_t bcc_entropy_from_random(
    uint8_t number_of_words,
    uint8_t (*random_generator)(),
    bcc_entropy *entropy,
    uint32_t *entropy_size
);

/*!
* delete the allocated memory of entropy byte array
* \param [in] entropy the entropy array
* \param [in] entropy_size the length of the entropy array
* \sa bcc_entropy_from_random()
* \sa bcc_entropy_from_english_mnemonics()
*/
void bcc_delete_entropy_array(uint8_t *entropy, uint32_t entropy_size);

/*!
* \brief encode a entropy into its equivalent words represented by their index (0 to 2047) in the BIP39 dictionary
* \param [in] entropy_raw A pointer to a byte array of either 16, 20, 24, 28 or 32 bytes
* \param [in] entropy_size of the entropy array
* \param [out] mnemonic_index the indexes of the encoded words  
* \param [in] mnemonic_size the number of encoded words 
* \returns success or failure 
*/
bcc_result bcc_bip39_encode(const char * const entropy_raw, unsigned long entropy_size, unsigned short *mnemonic_index, unsigned long mnemonic_size);

/*********/
/* Keys  */
/*********/

/*!
* Size of bcc_xprv
* Extended secret key (64 bytes) followed by a chain code (32 bytes)
* \sa bcc_xprv()
*/
#define XPRV_SIZE 96

/*!
* HDWallet extended private key
*
* Effectively this is ed25519 extended secret key (64 bytes) followed by a chain code (32 bytes)
*
*/
typedef struct bcc_xprv bcc_xprv;

/*!
* Extended Public Key (Point + ChainCode)
*/
typedef struct bcc_xpub bcc_xpub;

/*!
* BIP32 private to private derivation
*/
bcc_xprv *bcc_xprv_derive(bcc_xprv *xprv, uint32_t index); 

/*!
* Free the associated memory
*/
void bcc_xprv_delete(bcc_xprv *privkey);

/*!
* Get the associated bcc_xpub
*/
bcc_xpub *bcc_xprv_to_xpub(bcc_xprv *privkey);

/*!
* Get the bytes representation of bcc_xprv
* \sa bcc_xprv_bytes_delete
*/
uint8_t *bcc_xprv_to_bytes(bcc_xprv *privkey);

/*!
* Free the memory allocated with `bcc_xprv_to_bytes`
*/
void bcc_xprv_bytes_delete(uint8_t  *bytes);

/*!
* \brief Construct bcc_xprv from the given bytes
* \returns 1 if the representation is invalid 0 otherwise
* \sa bcc_xprv_delete
*/
bcc_result bcc_xprv_from_bytes(uint8_t *bytes, bcc_xprv **xprv_out);

/*!
* Free the associated memory
*/
void bcc_xpub_delete(bcc_xpub *pubkey);

/*************/
/* addresses */
/*************/

typedef struct bcc_address bcc_address;

/*! check if an address is a valid protocol address.
 * return 0 on success, !0 on failure. */
int bcc_address_is_valid(const char * address_base58);

bcc_address *bcc_address_new_from_pubkey(bcc_xpub *publickey);
void bcc_address_delete(bcc_address *address);

char *bcc_address_export_base58(bcc_address *address);
bcc_address *bcc_address_import_base58(const char * address_bytes);

/***********/
/* Wallet  */
/***********/

/*!
* HD BIP44 compliant wallet
*/
typedef struct bcc_wallet bcc_wallet;
typedef struct bcc_account bcc_account;

/*!
* Create a wallet with a seed generated from the given entropy and password. 
* The password can be empty and can be used to benefit from plausible deniability
* \param [in] entropy_ptr A pointer to a uint8_t array of either 16, 20, 24, 28 or 32 bytes
* \param [in] entropy_size The former size of the entropy array
* \param [in] password_ptr  A string with the password
* \param [in] password_size The size of the password string
* \param [out] wallet pointer to the created bcc_wallet that must be freed with `bcc_wallet_delete`
* \returns BCC_RESULT_SUCCESS | BCC_RESULT_ERROR if the entropy is of an invalid size
*/
bcc_result bcc_wallet_new(const uint8_t * const entropy_ptr, unsigned long entropy_size,
                                   const char * const password_ptr, unsigned long password_size,
                                   bcc_wallet** wallet);
/*!
* Free the memory of a wallet allocated with `bcc_wallet_new`
*/
void bcc_wallet_delete(bcc_wallet *);

/*!
* Get the wallet root xprv
* \param [in] wallet_ptr a wallet constructed with `bcc_wallet_new()`
* Call `bcc_xprv_delete` to deallocate the memory 
* \sa bcc_xprv_delete()
*/
bcc_xprv *bcc_wallet_root_key(bcc_wallet *wallet_ptr);

/*!
* \brief Create a new account, the account is given an alias and an index.
*
* The index is the derivation index, we do not check if there is already
* an account with this given index.
* The alias here is only an handy tool, to retrieve a created account from a wallet,
* it's not used for the account derivation process.
*
* \param [in] wallet A pointer to a wallet created with `bcc_wallet_new` 
* \param [in] alias A C string that can be used to retrieve an account from a wallet
* \param [in] index The derivation key 
* \returns pointer to the created account that must be freed with `bcc_account_delete` 
*/
bcc_account *bcc_account_create(bcc_wallet *wallet, const char *alias, unsigned int index);

/*!
* Free the memory allocated with `bcc_account_create`
* \param [in] account a pointer to the account to delete
*/
void bcc_account_delete(bcc_account *account);

/*!
* \brief Generate addressess
* The generated addresses are C strings in base58
* \param [in] account an account created with `bcc_account_create`
* \param [in] internal !0 for external address, 0 for internal 
* \param [in] from_index  
* \param [in] num_indices
* \param [out] addresses_ptr array of strings consisting of the base58 representation of the addresses
* \param [in] protocol_magic
* \returns the number of generated addresses
* \sa bcc_address_import_base58()
* \sa bcc_address_delete() 
*/
unsigned long bcc_account_generate_addresses(bcc_account *account, int internal, unsigned int from_index, unsigned long num_indices, char *addresses_ptr[], uint32_t protocol_magic);
void bcc_account_delete_addresses(char *addresses_ptr[], unsigned long length);

/****************/
/* Transactions */
/****************/

/* transaction error definitions */
typedef enum _transaction_config_error
{
    BCC_TRANSACTION_SUCCESS = 0,
    BCC_TRANSACTION_NO_OUTPUT = 1,
    BCC_TRANSACTION_NO_INPUT = 2,
    /*!The number of signatures doesn't match the number of inputs*/
    BCC_TRANSACTION_SIGNATURE_MISMATCH = 3,
    /*!The transaction is too big*/
    BCC_TRANSACTION_OVER_LIMIT = 4,
    /*!The number of signatures is greater than the number of inputs*/
    BCC_TRANSACTION_SIGNATURES_EXCEEDED = 5,
    /*!The given value is greater than the maximum allowed coin value*/
    BCC_TRANSACTION_COIN_OUT_OF_BOUNDS = 6,
} bcc_transaction_error_t;

typedef struct bcc_transaction_builder bcc_transaction_builder;
typedef struct bcc_transaction_finalized bcc_transaction_finalized;
/*!
* Used for addressing a specific output of a transaction built from a TxId (hash of the tx) and the offset in the outputs of this transaction.
* \sa bcc_transaction_output_ptr_new()
*/
typedef struct bcc_txoptr bcc_txoptr;
/*!
* Used for representing a transaction's output
* \sa bcc_transaction_output_new()
*/
typedef struct bcc_txoutput bcc_txoutput;
typedef struct bcc_transaction bcc_transaction;

/*!
* \struct bcc_signed_transaction
*/
typedef struct bcc_signed_transaction bcc_signed_transaction;

//Type for enforcing array size
typedef struct bcc_txid {
    uint8_t bytes[32];
} bcc_txid_t;

/*! \brief Get the transaction id
* \param [in] txaux
* \param [out] out_txid
* \relates bcc_signed_transaction
*/
void bcc_signed_transaction_txid(
    bcc_signed_transaction *txaux,
    bcc_txid_t *out_txid
);

/*! \brief Get the transaction id
* \param [in] tx
* \param [out] out_txid
* \relates bcc_transaction
*/
void bcc_transaction_txid(
    bcc_transaction *tx,
    bcc_txid_t *out_txid
);

/*! \brief Get references to the inputs in the the signed transaction
* \param [in] txaux a bcc signed transaction
* \param [out] out_array array of pointers to bcc_txoptr that you can read with 
* `bcc_transaction_txoptr_txid` and `bcc_transaction_txoptr_index`.
* You should NOT call bcc_transaction_output_ptr_delete with any of this pointers.
* <br> Use `bcc_signed_transaction_delete_inputs` to delete the array of pointers
* \param [out] out_size
* \sa bcc_signed_transaction_delete_inputs()
* \sa bcc_transaction_txoptr_txid()
* \sa bcc_transaction_txoptr_index()
* \sa bcc_signed_transaction_get_outputs()
*/
void bcc_signed_transaction_get_inputs(
    bcc_signed_transaction *txaux,
    bcc_txoptr *(*out_array[]),
    size_t *out_size
);

/*! \brief Release the memory allocated by `bcc_transaction_signed_get_inputs`
*/
void bcc_signed_transaction_delete_inputs(bcc_txoptr *inputs[], size_t size);

/*! \brief Get references to the outputs in the the signed transaction
* \param [in] txaux a bcc signed transaction
* \param [out] out_array array of pointers to bcc_txoutput that you can read with 
`bcc_transaction_txoutput_address` and `bcc_transaction_txoptr_value`.
You should not call bcc_transaction_output_delete with any of this pointers
Use `bcc_transaction_signed_delete_inputs` to delete the array of pointers
* \param [out] out_size
* \sa bcc_signed_transaction_delete_outputs()
* \sa bcc_transaction_txoutput_address()
* \sa bcc_transaction_txoutput_value()
* \sa bcc_signed_transaction_get_inputs()
*/
void bcc_signed_transaction_get_outputs(
    bcc_signed_transaction *txaux,
    bcc_txoutput *(*outputs[]),
    size_t *outputs_size
);

/*! \brief Encode a bcc_signed_transaction as CBOR 
* \param [in] txaux a bcc signed transaction
* \param [out] out_ptr pointer to the serialized data
* \param [out] out_size size of the encoded data
* \sa bcc_signed_transaction_serialized_delete()
*/
bcc_result bcc_signed_transaction_serialize(
    bcc_signed_transaction *txaux,
    uint8_t **out_ptr,
    size_t *out_size);

/*! \brief Release the memory allocated by bcc_signed_transaction_serialize()
* \param [in] pointer
* \param [in] size
*/
void bcc_signed_transaction_serialized_delete(uint8_t *pointer, size_t size);

/*! \brief Release the memory allocated by `bcc_transaction_signed_get_outputs`
*/
void bcc_signed_transaction_delete_outputs(bcc_txoutput *outputs[], size_t size);

/*!
* Create object used for addressing a specific output of a transaction built from a TxId (hash of the tx) and the offset in the outputs of this transaction.
* The memory must be freed with bcc_transaction_output_ptr_delete
* \sa bcc_transaction_output_ptr_delete()
*/
bcc_txoptr * bcc_transaction_output_ptr_new(uint8_t txid[32], uint32_t index);

/*!
* Free the memory allocated with `bcc_transaction_output_ptr_new`
*/
void bcc_transaction_output_ptr_delete(bcc_txoptr *txo);


/*!
* Get the txid of the given bcc_txoptr
* \param [in] txoptr
* \param [out] output
*/
void bcc_transaction_txoptr_txid(bcc_txoptr *txoptr, bcc_txid_t *output);

/*!
* Get the index of the given bcc_txoptr
*/
uint32_t bcc_transaction_txoptr_index(bcc_txoptr *txoptr);

/*!
* Create output for a transaction 
* The memory must be freed with `bcc_transaction_output_delete`
* \sa bcc_transaction_output_delete()
*/
bcc_txoutput * bcc_transaction_output_new(bcc_address *c_addr, uint64_t value);

/*!
* Free the memory allocated with `bcc_transaction_output_delete`
*/
void bcc_transaction_output_delete(bcc_txoutput *output);

/*!
* \brief get a reference to the address of a bcc_txoutput
* You should NOT `bcc_address_delete` on the pointer returned by this function 
* as the data is this function doesn't allocate memory, and the returned pointer is only
* valid as long as the bcc_txoutput is valid
*/
bcc_address *bcc_transaction_txoutput_address(bcc_txoutput *txoutput);

/*!
* \brief get a reference to the address of a bcc_txoutput
*/
uint64_t bcc_transaction_txoutput_value(bcc_txoutput *txoutput);

/*!
* \brief Create builder for a transaction
* \returns builder object
* \sa bcc_transaction_builder_delete()
* \sa bcc_transaction_builder_add_output()
* \sa bcc_transaction_builder_add_input()
* \sa bcc_transaction_builder_add_change_addr()
* \sa bcc_transaction_builder_fee()
* \sa bcc_transaction_builder_finalize()
*/
bcc_transaction_builder * bcc_transaction_builder_new(void);

/*!
* \brief Delete bcc_transaction_builder and free the associated memory
*/
void bcc_transaction_builder_delete(bcc_transaction_builder *tb);

/*!
* \brief Add output to transaction
* \param [in] tb the builder for the transaction
* \param [in] txo created with `bcc_transaction_output_new`
* \sa bcc_transaction_output_new()
*/
void bcc_transaction_builder_add_output(bcc_transaction_builder *tb, bcc_txoutput *txo);

/*!
* \brief Add input to the transaction
* \param [in] tb the builder for the transaction
* \param [in] c_txo created with `bcc_transaction_output_ptr_new`
* \param [in] value the cost 
* \sa bcc_transaction_output_ptr_new()
* \returns BCC_TRANSACTION_SUCCESS | BCC_TRANSACTION_COIN_OUT_OF_BOUNDS
*/
bcc_transaction_error_t bcc_transaction_builder_add_input(bcc_transaction_builder *tb, bcc_txoptr *c_txo, uint64_t value);

/*!
* \brief This associate all the leftover values, if any to an output with the specified address.
*
* If the transaction is already consuming all inputs in its outputs (perfectly balanced),
* then it has no effect
*
* If there's not enough inputs value compared to the existing outputs, then a failure status is returned.
* If there's no way to "fit" the output policy in the transaction building, as the fee cannot cover
* the basic overhead, then a failure status is returned.
*
* Note: that the calculation is not done again if more inputs and outputs are added after this call,
* and in most typical cases this should be the last addition to the transaction.
*
* \param [in] tb the builder for the transaction
* \param [in] change_addr used for the change (leftover values) output 
* \returns 0 for success 0! for failure
*/
bcc_result bcc_transaction_builder_add_change_addr(bcc_transaction_builder *tb, bcc_address *change_addr);

/*!
* \brief Calculate the fee for the transaction with a linear algorithm
* \returns fee
*/
uint64_t bcc_transaction_builder_fee(bcc_transaction_builder *tb);

/*!
* struct for representing the sign in bcc_transaction_coin_diff_t
* \sa bcc_transaction_coin_diff
*/
typedef enum difference_type {
    DIFF_POSITIVE,
    DIFF_NEGATIVE,
    DIFF_ZERO,
} difference_type_t;

/*!
* struct for repressenting a balance returned with bcc_transaction_builder_balance and bcc_transaction_builder_balance_without_fees
* \sa bcc_transaction_builder_balance() bcc_transaction_builder_balance_without_fees()
*/
typedef struct bcc_transaction_coin_diff {
    difference_type_t sign;
    uint64_t value;
} bcc_transaction_coin_diff_t;

/*!
* \brief Deallocate the memory allocated with `bcc_transaction_builder_balance` and `bcc_transaction_builder_balance_without_fees`
* \sa bcc_transaction_builder_balance() bcc_transaction_builder_balance_without_fees()
*/
void bcc_transaction_balance_delete(bcc_transaction_coin_diff_t *balance);

/*!
* Try to return the differential between the outputs (including fees) and the inputs
* \param [in] tb the builder for the transaction
* \param [out] out a pointer to a bcc_transaction_coin_diff_t where: 
*   - (sign == DIFF_ZERO) means we have a balanced transaction where inputs === outputs
*   - (sign == DIFF_NEGATIVE) means (outputs+fees) > inputs. More inputs required.
*   - (sign == DIFF_POSITIVE) means inputs > (outputs+fees). 
*   .
* and the value field indicates the quantity (in -1 and 1 cases)
* Excessive input goes to larger fee.
* \returns BCC_TRANSACTION_SUCCESS | BCC_TRANSACTION_COIN_OUT_OF_BOUNDS if the total is too big
* \sa bcc_transaction_balance_delete()
*/
bcc_transaction_error_t bcc_transaction_builder_balance(bcc_transaction_builder *tb, bcc_transaction_coin_diff_t **out);

/*!
* Try to return the differential between the outputs (excluding fees) and the inputs
* \param [in] tb the builder for the transaction
* \param [out] out a pointer to a bcc_transaction_coin_diff_t where:
*   - (sign == DIFF_ZERO) means we have a balanced transaction where inputs === outputs
*   - (sign == DIFF_NEGATIVE) means (outputs) > inputs. More inputs required.
*   - (sign == DIFF_POSITIVE) means inputs > (outputs). 
*   .
* and the value field indicates the quantity (in -1 and 1 cases)
* \returns BCC_TRANSACTION_SUCCESS | BCC_TRANSACTION_COIN_OUT_OF_BOUNDS if the total is too big
* \sa bcc_transaction_balance_delete()
*/
bcc_transaction_error_t bcc_transaction_builder_balance_without_fees(bcc_transaction_builder *tb, bcc_transaction_coin_diff_t **out);

/*!
* Try to return the sum of the inputs
* \param [in] tb the builder for the transaction
* \param [out] output the sum
* \returns BCC_TRANSACTION_SUCCESS | BCC_TRANSACTION_COIN_OUT_OF_BOUNDS if the total is too big
*/
bcc_transaction_error_t bcc_transaction_builder_get_input_total(bcc_transaction_builder *tb, uint64_t *output);

/*!
* Try to return the sum of the outputs
* \param [in] tb the builder for the transaction
* \param [out] output the sum
* \returns BCC_TRANSACTION_SUCCESS | BCC_TRANSACTION_COIN_OUT_OF_BOUNDS if the total is too big
*/
bcc_transaction_error_t bcc_transaction_builder_get_output_total(bcc_transaction_builder *tb, uint64_t *output);

/*!
* \brief Get a transaction object
* \returns BCC_TRANSACTION_SUCCESS | BCC_TRANSACTION_NO_INPUT | BCC_TRANSACTION_NO_OUTPUT
*/
bcc_transaction_error_t bcc_transaction_builder_finalize(bcc_transaction_builder *tb, bcc_transaction **tx);
void bcc_transaction_delete(bcc_transaction *c_tx);

/*!
* \brief Take a transaction and create a working area for adding witnesses
*/
bcc_transaction_finalized * bcc_transaction_finalized_new(bcc_transaction *c_tx);
void bcc_transaction_finalized_delete(bcc_transaction_finalized *tf);

/*!
* Add a witness associated with the next input.
*
* Witness need to be added in the same order to the inputs, otherwise protocol level mismatch will happen, and the transaction will be rejected
* \param tf a transaction finalized 
* \param c_xprv
* \param protocol_magic
* \param c_txid
* \sa bcc_transaction_builder_new
* \returns BCC_TRANSACTION_SUCCESS | BCC_TRANSACTION_SIGNATURES_EXCEEDED
*/
bcc_transaction_error_t bcc_transaction_finalized_add_witness(bcc_transaction_finalized *tf, uint8_t c_xprv[96], uint32_t protocol_magic, uint8_t c_txid[32]);

/*!
* \brief A finalized transaction with the vector of witnesses
* \param tf a finalized transaction with witnesses
* \sa bcc_transaction_finalized_add_witness()
* \returns BCC_TRANSACTION_SUCCESS | BCC_TRANSACTION_SIGNATURE_MISMATCH | BCC_TRANSACTION_OVER_LIMIT
*/
bcc_transaction_error_t bcc_transaction_finalized_output(bcc_transaction_finalized *tf, bcc_signed_transaction **txaux);
void bcc_transaction_signed_delete(bcc_signed_transaction *txaux);

/****************/
/* Block */
/****************/

typedef struct bcc_block bcc_block;

/*! \brief Try to the decode a CBOR encoded block
* \param [in] bytes the block in its raw representation
* \param [in] size the size in bytes of the binary 
* \param [out] out_block a pointer to the block, you should call `bcc_block_delete` on this pointer
* \returns error if the binary format is wrong 
*/
bcc_result bcc_raw_block_decode(const uint8_t *bytes, size_t size, bcc_block **out_block);

/*! \brief delete memory allocated by `bcc_raw_block_decode`
*/
void bcc_block_delete(bcc_block *block);

/*! \brief Get references to the block transactions
* \param [in] block the corresponding block
* \param [out] out_array this array contains pointers that you can use for 
bcc_signed_transaction_get_inputs() and bcc_signed_transaction_get_outputs()
you should NOT call `bcc_transaction_signed_delete`() on any of this pointers.
You should call bcc_block_delete_transactions to free the memory allocated for this pointers
* \param [out] size the number of transactions
* \returns BCC_RESULT_ERROR if the block is a BoundaryBlock
* \sa bcc_signed_transaction_get_inputs()
* \sa bcc_signed_transaction_get_outputs()
* \sa bcc_block_delete_transactions()
*/
bcc_result bcc_block_get_transactions(
    bcc_block *block,
    bcc_signed_transaction *(*out_array[]),
    size_t *size
);

/*! \brief Release the memory allocated by bcc_block_get_transactions
*/
void bcc_block_delete_transactions(
    bcc_signed_transaction *transactions[], size_t size
);

/*!
* \struct bcc_block_header
* \brief Opaque handler for the Header of a Block
*
* Struct for working with block headers, which can be used to obtain the id of the respective and previous blocks
*/
typedef struct bcc_block_header bcc_block_header;

/*!
*\brief Get a copy of the given block's header
*\param [in] block
*\returns a pointer to a heap allocated copy of the header inside the block.
*You should call `bcc_block_header_delete()` to release the memory
*\relatesalso bcc_block_header
*/
bcc_block_header *bcc_block_get_header(bcc_block *block);

/*!
*\brief Decode a header in its cbor representation
*\param [in] bytes buffer
*\param [in] size size of the given buffer
*\param [out] out_header a pointer to the header
*You should call `bcc_block_header_delete()` to release the memory
*\relatesalso bcc_block_header
*/
bcc_result bcc_raw_block_header_decode(
    const uint8_t *bytes, size_t size, bcc_block_header **out_header);

/*!
*\brief Get the hash (as a string in hexaedecimal representation) of the previous header (which is the previous blockid)
*\param [in] header pointer to the header obtained with `bcc_block_get_header()` or `bcc_raw_block_header_decode()`
*\relatesalso bcc_block_header
*\sa `bcc_block_delete_hash()`
*/
char *bcc_block_header_previous_hash(bcc_block_header *header);

/*!
*\brief get the hash (in hexadecimal representation) of a given header, that can be used as the blockid
*\param [in] header
*\relatesalso bcc_block_header
*\sa `bcc_block_delete_hash()`
*/
char *bcc_block_header_compute_hash(bcc_block_header *header);

/*!
*\brief release the memory allocated with bcc_block_header_previous_hash 
*    or bcc_block_header_compute_hash
*\param [in] hash
*\relatesalso bcc_block_header_previous_hash
*\relatesalso bcc_block_header_compute_hash
*/
void bcc_block_delete_hash(char *hash);

/*!
*\brief release the memory allocated with `bcc_block_get_header` and `bcc_raw_block_header_decode`
*\param [in] hash
*\relatesalso bcc_block_header
*/
void bcc_block_header_delete(bcc_block_header *header);

#ifdef __cplusplus
}
#endif

#endif
