# Creating a wallet

This example shows how to create a wallet from english mnemonics

```C
char *MNEMONICS = "crowd captain hungry tray powder motor coast oppose month shed parent mystery torch resemble index";

/*Retrieve entropy from mnemonics*/
bcc_entropy entropy;
uint32_t bytes;
bcc_bip39_error_t entropy_rc = bcc_entropy_from_english_mnemonics(MNEMONICS, &entropy, &bytes);

/*Check that the mnemonics were actually valid*/
assert(entropy_rc == BIP39_SUCCESS);

/*Create a wallet with the given entropy*/
char *password = "password";
bcc_wallet *wallet;
bcc_result wallet_rc = bcc_wallet_new(entropy, bytes, password, strlen(password), &wallet);

assert(wallet_rc == BCC_RESULT_SUCCESS);

/*Create an account*/
const char *alias = "Awesome Account";
unsigned int index = 0;
bcc_account *account = bcc_account_create(wallet, alias, index);

/*Create an internal address*/
enum
{
    NUMBER_OF_ADDRESSES = 1,
};
char *address[NUMBER_OF_ADDRESSES];
const int IS_INTERNAL = 1;
const unsigned int FROM_INDEX = 0;
bcc_account_generate_addresses(account, IS_INTERNAL, FROM_INDEX, NUMBER_OF_ADDRESSES, address, PROTOCOL_MAGIC);

/*
    ...
*/

/*Release memory*/
bcc_delete_entropy_array(entropy, bytes);

bcc_account_delete_addresses(address, NUMBER_OF_ADDRESSES);

bcc_account_delete(account);

bcc_wallet_delete(wallet);
```

# [Http bridge](https://github.com/the-blockchain-company/bcc-http-bridge) integration

To read the transactions on the blockchain, one can query the blocks from the bridge in its raw form and then use the C bindings to read this data. For example:
## Decoding a block

```C
/*main.c*/
/* ... */
#include "bcc.h"

/*
Assumming that raw_block is an array of bytes containing the data
obtained from the http bridge with the GET /:network/block/:blockid endpoint
and the raw_block_size contains the respective size of this buffer

char *raw_block;
size_t raw_block_size;
*/

int main(int argc, char *argv[]) {
    bcc_block *block;
    bcc_raw_block_decode(raw_block, raw_block_size, &block);
    //free(raw_block);

    print_block(block);
    return 0;
}
```

## Printing block information

```C
void print_block(bcc_block *block) {
    bcc_block_header *header = bcc_block_get_header(block);

    char *hash = bcc_block_header_compute_hash(header);
    printf("Block id: %s\n", hash);
    bcc_block_delete_hash(hash);

    char *previous_hash = bcc_block_header_previous_hash(header);
    printf("Previous block id: %s\n", previous_hash);
    bcc_block_delete_hash(previous_hash);

    bcc_block_header_delete(header);

    size_t transactions_size;
    bcc_signed_transaction **transactions;
    bcc_result rc = bcc_block_get_transactions(block, &transactions, &transactions_size);

    assert(rc == BCC_RESULT_SUCCESS);

    printf("Transactions: (%zu)\n", transactions_size);
    for (unsigned int i = 0; i < transactions_size; ++i)
    {
        print_transaction(transactions[i]);
    }

    bcc_block_delete_transactions(transactions, transactions_size);
    bcc_block_delete(block);
}
```

## Printing transactions

```C
void print_transaction(bcc_signed_transaction *tx) {
    print_inputs(tx);
    print_outputs(tx);
}
```

```C
void print_inputs(bcc_signed_transaction *tx)
{
    printf("Inputs\n");
    bcc_txoptr **inputs;
    size_t inputs_size;

    bcc_signed_transaction_get_inputs(tx, &inputs, &inputs_size);

    for (unsigned int i = 0; i < inputs_size; ++i)
    {
        uint32_t index = bcc_transaction_txoptr_index(inputs[i]);
        bcc_txid_t txid;
        bcc_transaction_txoptr_txid(inputs[i], &txid);

        /*Print the array of bytes as a hex string*/
        printf("Txid:");
        for (unsigned int j = 0; j < sizeof(txid); ++j)
        {
            printf("%02x", txid.bytes[j]);
        }
        printf("\n");

        /*The index in the tx*/
        printf("Offset %d\n", index);
    }
    bcc_signed_transaction_delete_inputs(inputs, inputs_size);
}
```

```C
void print_outputs(bcc_signed_transaction *tx)
{
    printf("Outputs\n");
    bcc_txoutput **outputs;
    size_t outputs_size;

    bcc_signed_transaction_get_outputs(tx, &outputs, &outputs_size);

    for (unsigned int i = 0; i < outputs_size; ++i)
    {
        bcc_address *address = bcc_transaction_txoutput_address(outputs[i]);
        char *address_base58 = bcc_address_export_base58(address);
        uint64_t value = bcc_transaction_txoutput_value(outputs[i]);
        printf("Value: %" PRIu64 "\n", value);
        printf("Address: %s\n", address_base58);
        bcc_account_delete_addresses(&address_base58, 1);
    }

    bcc_signed_transaction_delete_outputs(outputs, outputs_size);
}
```
