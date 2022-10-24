#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../bcc.h"
#include "unity/unity.h"

//Variables for the setUp function
bcc_wallet *wallet;
bcc_account *account;
bcc_address *input_address;
bcc_address *output_address;
bcc_transaction_builder *txbuilder;
bcc_txoptr *input;
bcc_txoutput *output;

//Constants
static uint32_t PROTOCOL_MAGIC = 1;
static uint8_t input_xprv[XPRV_SIZE] = {0};
static const uint8_t static_wallet_entropy[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static uint8_t txid[32] = {0};
const uint64_t MAX_COIN = 45000000000000000;

void setUp()
{
    bcc_result wallet_rc = bcc_wallet_new(
        static_wallet_entropy,
        sizeof(static_wallet_entropy),
        "password",
        strlen("password"),
        &wallet);

    account = bcc_account_create(wallet, "main", 0);

    char *addresses[2];
    size_t NUMBER_OF_ADDRESSES = sizeof(addresses) / sizeof(char *);

    int rc = bcc_account_generate_addresses(account, 0, 0, NUMBER_OF_ADDRESSES, addresses, PROTOCOL_MAGIC);

    input_address = bcc_address_import_base58(addresses[0]);
    output_address = bcc_address_import_base58(addresses[1]);

    bcc_account_delete_addresses(addresses, sizeof(addresses) / sizeof(char *));

    txbuilder = bcc_transaction_builder_new();
    
    input = bcc_transaction_output_ptr_new(txid, 1);
    output = bcc_transaction_output_new(output_address, 1000);
}

void tearDown()
{
    bcc_transaction_output_delete(output);

    bcc_transaction_output_ptr_delete(input);

    bcc_wallet_delete(wallet);

    bcc_transaction_builder_delete(txbuilder);

    bcc_address_delete(input_address);

    bcc_address_delete(output_address);

    bcc_account_delete(account);
}

void test_add_input_returns_success_with_valid_value()
{
    bcc_transaction_error_t irc = bcc_transaction_builder_add_input(txbuilder, input, 1000);

    TEST_ASSERT_EQUAL(BCC_RESULT_SUCCESS, irc);
}

void test_add_input_returns_error_with_big_value()
{
    bcc_transaction_error_t irc = bcc_transaction_builder_add_input(txbuilder, input, MAX_COIN + 1);

    TEST_ASSERT_EQUAL(BCC_TRANSACTION_COIN_OUT_OF_BOUNDS, irc);
}

void test_add_witness_returns_error_with_less_inputs()
{
    bcc_result irc = bcc_transaction_builder_add_input(txbuilder, input, 1000);

    /* the builder finalize fails without outputs*/
    bcc_transaction_builder_add_output(txbuilder, output);

    bcc_transaction *tx; bcc_transaction_error_t tx_rc = bcc_transaction_builder_finalize(txbuilder, &tx);

    TEST_ASSERT_EQUAL(BCC_TRANSACTION_SUCCESS, tx_rc);

    bcc_transaction_finalized *tf = bcc_transaction_finalized_new(tx);

    bcc_transaction_error_t rc1 = bcc_transaction_finalized_add_witness(tf, input_xprv, PROTOCOL_MAGIC, txid);

    TEST_ASSERT_EQUAL(BCC_TRANSACTION_SUCCESS, rc1);

    bcc_transaction_error_t rc2 = bcc_transaction_finalized_add_witness(tf, input_xprv, PROTOCOL_MAGIC, txid);

    //#witnesses > #inputs
    TEST_ASSERT_EQUAL(BCC_TRANSACTION_SIGNATURES_EXCEEDED, rc2);

    bcc_transaction_delete(tx);
    bcc_transaction_finalized_delete(tf);
}

void test_builder_finalize_error_code_no_inputs()
{
    bcc_transaction_builder_add_output(txbuilder, output);

    bcc_transaction *tx;
    bcc_transaction_error_t tx_rc = bcc_transaction_builder_finalize(txbuilder, &tx);
    TEST_ASSERT_EQUAL(BCC_TRANSACTION_NO_INPUT, tx_rc);
}

void test_builder_finalize_error_code_no_outputs()
{
    bcc_transaction_error_t irc = bcc_transaction_builder_add_input(txbuilder, input, 1000);

    bcc_transaction *tx;
    bcc_transaction_error_t tx_rc = bcc_transaction_builder_finalize(txbuilder, &tx);
    TEST_ASSERT_EQUAL(BCC_TRANSACTION_NO_OUTPUT, tx_rc);
}

void test_transaction_finalized_output_error_code_signature_mismatch()
{
    bcc_transaction_error_t irc1 = bcc_transaction_builder_add_input(txbuilder, input, 1000);
    bcc_transaction_error_t irc2 = bcc_transaction_builder_add_input(txbuilder, input, 1000);

    bcc_transaction_builder_add_output(txbuilder, output);

    bcc_transaction *tx;
    bcc_transaction_error_t tx_rc = bcc_transaction_builder_finalize(txbuilder, &tx);

    bcc_transaction_finalized *tf = bcc_transaction_finalized_new(tx);

    bcc_transaction_error_t rc1 = bcc_transaction_finalized_add_witness(tf, input_xprv, PROTOCOL_MAGIC, txid);

    bcc_signed_transaction *txaux;
    bcc_transaction_error_t rc = bcc_transaction_finalized_output(tf, &txaux);

    //#inputs (2) > #witnesses (1)
    TEST_ASSERT_EQUAL(BCC_TRANSACTION_SIGNATURE_MISMATCH, rc);

    bcc_transaction_delete(tx);
    bcc_transaction_finalized_delete(tf);
}

void test_transaction_finalized_output_success()
{
    bcc_transaction_error_t irc1 = bcc_transaction_builder_add_input(txbuilder, input, 1000);
    bcc_transaction_builder_add_output(txbuilder, output);

    bcc_transaction *tx;
    bcc_transaction_error_t tx_rc = bcc_transaction_builder_finalize(txbuilder, &tx);

    bcc_transaction_finalized *tf = bcc_transaction_finalized_new(tx);

    bcc_transaction_error_t rc1 = bcc_transaction_finalized_add_witness(tf, input_xprv, PROTOCOL_MAGIC, txid);

    bcc_signed_transaction *txaux;
    bcc_transaction_error_t rc = bcc_transaction_finalized_output(tf, &txaux);

    TEST_ASSERT_EQUAL(BCC_TRANSACTION_SUCCESS, rc);

    bcc_transaction_delete(tx);
    bcc_transaction_finalized_delete(tf);
    bcc_transaction_signed_delete(txaux);
}

void test_transaction_balance_positive() {
    bcc_transaction_coin_diff_t *balance;

    bcc_transaction_builder_add_input(txbuilder, input, 1000000);

    bcc_transaction_error_t rc = bcc_transaction_builder_balance(txbuilder, &balance);
    uint64_t fee = bcc_transaction_builder_fee(txbuilder);

    TEST_ASSERT_EQUAL(1000000 - fee, (*balance).value);
    //TEST_ASSERT_EQUAL(DIFF_POSITIVE, (*balance).sign);
}

void test_transaction_balance_negative() {
    bcc_transaction_coin_diff_t *balance;
    bcc_transaction_error_t rc = bcc_transaction_builder_balance(txbuilder, &balance);

    uint64_t fee = bcc_transaction_builder_fee(txbuilder);

    TEST_ASSERT_EQUAL(fee, (*balance).value);
    TEST_ASSERT_EQUAL(DIFF_NEGATIVE, (*balance).sign);
}

void test_transaction_balance_zero() {
    enum {
        BIG_VALUE_TO_COVER_FEE = 10000000,
    };
    bcc_transaction_builder_add_input(txbuilder, input, BIG_VALUE_TO_COVER_FEE);
    bcc_result add_change_rc = bcc_transaction_builder_add_change_addr(txbuilder, output_address);

    bcc_transaction_coin_diff_t *balance;
    bcc_transaction_error_t rc = bcc_transaction_builder_balance(txbuilder, &balance);

    TEST_ASSERT_EQUAL(0, (*balance).value);
    TEST_ASSERT_EQUAL(DIFF_ZERO, (*balance).sign);
}

void test_transaction_builder_balance_too_big() {
    bcc_txoptr *input1 = bcc_transaction_output_ptr_new(txid, 1);
    bcc_txoptr *input2 = bcc_transaction_output_ptr_new(txid, 2);

    bcc_result irc1 = bcc_transaction_builder_add_input(txbuilder, input1, MAX_COIN);
    bcc_result irc2 = bcc_transaction_builder_add_input(txbuilder, input1, 1);

    bcc_transaction_coin_diff_t *balance; 
    bcc_transaction_error_t brc1 = bcc_transaction_builder_balance(txbuilder, &balance);

    TEST_ASSERT_EQUAL(BCC_TRANSACTION_COIN_OUT_OF_BOUNDS, brc1);

    bcc_transaction_output_ptr_delete(input1);
    bcc_transaction_output_ptr_delete(input2);
}

void test_transaction_builder_balance_without_fee_too_big() {
    bcc_txoptr *input1 = bcc_transaction_output_ptr_new(txid, 1);
    bcc_txoptr *input2 = bcc_transaction_output_ptr_new(txid, 2);

    bcc_result irc1 = bcc_transaction_builder_add_input(txbuilder, input1, MAX_COIN);
    bcc_result irc2 = bcc_transaction_builder_add_input(txbuilder, input1, 1);

    bcc_transaction_coin_diff_t *balance; 
    bcc_transaction_error_t brc1 = bcc_transaction_builder_balance_without_fees(txbuilder, &balance);

    TEST_ASSERT_EQUAL(BCC_TRANSACTION_COIN_OUT_OF_BOUNDS, brc1);

    bcc_transaction_output_ptr_delete(input1);
    bcc_transaction_output_ptr_delete(input2);
}

void test_transaction_balance_without_fee_positive() {
    bcc_transaction_builder_add_input(txbuilder, input, 1000);
    bcc_transaction_coin_diff_t *balance;
    bcc_transaction_error_t rc = bcc_transaction_builder_balance_without_fees(txbuilder, &balance);

    TEST_ASSERT_EQUAL(1000, (*balance).value);
    TEST_ASSERT_EQUAL(DIFF_POSITIVE, (*balance).sign);
}

void test_transaction_balance_without_fee_negative() {
    bcc_txoutput *output = bcc_transaction_output_new(output_address, 1000);

    bcc_transaction_builder_add_output(txbuilder, output);
    bcc_transaction_coin_diff_t *balance;
    bcc_transaction_error_t rc = bcc_transaction_builder_balance_without_fees(txbuilder, &balance);

    TEST_ASSERT_EQUAL(1000, (*balance).value);
    TEST_ASSERT_EQUAL(DIFF_NEGATIVE, (*balance).sign);
    bcc_transaction_output_delete(output);
}

void test_transaction_balance_without_fee_zero() {
    bcc_txoutput *output = bcc_transaction_output_new(output_address, 1000);

    bcc_transaction_builder_add_input(txbuilder, input, 1000);
    bcc_transaction_builder_add_output(txbuilder, output);

    bcc_transaction_coin_diff_t *balance;
    bcc_transaction_error_t rc = bcc_transaction_builder_balance_without_fees(txbuilder, &balance);

    TEST_ASSERT_EQUAL(0, (*balance).value);
    TEST_ASSERT_EQUAL(DIFF_ZERO, (*balance).sign);
    bcc_transaction_output_delete(output);
}

void test_transaction_get_input_total() {
    bcc_transaction_error_t irc = bcc_transaction_builder_add_input(txbuilder, input, 1000);
    uint64_t input_total;
    bcc_transaction_error_t rc = bcc_transaction_builder_get_input_total(txbuilder, &input_total);
    TEST_ASSERT_EQUAL(1000, input_total);
}

void test_transaction_get_output_total() {
    bcc_transaction_builder_add_output(txbuilder, output);
    uint64_t output_total;
    bcc_transaction_error_t rc = bcc_transaction_builder_get_output_total(txbuilder, &output_total);
    TEST_ASSERT_EQUAL(1000, output_total);
}

void test_transaction_get_input_total_no_inputs() {
    uint64_t input_total;
    bcc_transaction_error_t rc = bcc_transaction_builder_get_input_total(txbuilder, &input_total);
    TEST_ASSERT_EQUAL(0, input_total);
}

void test_transaction_get_output_total_no_outputs() {
    uint64_t output_total;
    bcc_transaction_error_t rc = bcc_transaction_builder_get_output_total(txbuilder, &output_total);
    TEST_ASSERT_EQUAL(0, output_total);
}

void test_transaction_get_input_total_too_big()
{
    bcc_transaction_error_t irc1 = bcc_transaction_builder_add_input(txbuilder, input, MAX_COIN);
    bcc_transaction_error_t irc2 = bcc_transaction_builder_add_input(txbuilder, input, 1);
    TEST_ASSERT_EQUAL(BCC_TRANSACTION_SUCCESS, irc1);
    TEST_ASSERT_EQUAL(BCC_TRANSACTION_SUCCESS, irc2);

    uint64_t input_total;
    bcc_transaction_error_t rc = bcc_transaction_builder_get_input_total(txbuilder, &input_total);
    TEST_ASSERT_EQUAL(BCC_TRANSACTION_COIN_OUT_OF_BOUNDS, rc);
}

void test_transaction_get_output_total_too_big()
{
    bcc_txoutput *output1 = bcc_transaction_output_new(output_address, MAX_COIN);
    bcc_txoutput *output2 = bcc_transaction_output_new(output_address, 1);

    bcc_transaction_builder_add_output(txbuilder, output1);
    bcc_transaction_builder_add_output(txbuilder, output2);
    uint64_t output_total;
    bcc_transaction_error_t rc = bcc_transaction_builder_get_output_total(txbuilder, &output_total);
    TEST_ASSERT_EQUAL(BCC_TRANSACTION_COIN_OUT_OF_BOUNDS, rc);

    bcc_transaction_output_delete(output1);
    bcc_transaction_output_delete(output2);
}

void test_transaction_finalized_serialize()
{
    bcc_transaction_error_t irc1 = bcc_transaction_builder_add_input(txbuilder, input, 1000);
    bcc_transaction_builder_add_output(txbuilder, output);

    bcc_transaction *tx;
    bcc_transaction_error_t tx_rc = bcc_transaction_builder_finalize(txbuilder, &tx);

    bcc_transaction_finalized *tf = bcc_transaction_finalized_new(tx);

    bcc_transaction_error_t rc1 = bcc_transaction_finalized_add_witness(tf, input_xprv, PROTOCOL_MAGIC, txid);

    bcc_signed_transaction *txaux;
    bcc_transaction_error_t rc = bcc_transaction_finalized_output(tf, &txaux);

    uint8_t *bytes;
    size_t size;
    bcc_result status = bcc_signed_transaction_serialize(txaux, &bytes, &size);

    uint8_t actual_txaux[] = {
        130, 131, 159, 130, 0, 216, 24, 88, 36, 130, 88, 32, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 1, 255, 159, 130, 130, 216, 24, 88, 36, 131, 88, 28, 121, 249, 185, 75,
        10, 140, 75, 131, 137, 174, 29, 193, 190, 51, 24, 21, 69, 212, 76, 142, 123, 215, 231, 188,
        171, 83, 143, 85, 161, 2, 65, 1, 0, 26, 100, 143, 115, 160, 25, 3, 232, 255, 160, 129,
        130, 0, 216, 24, 88, 133, 130, 88, 64, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 88, 64, 10, 168, 244, 131, 6,
        4, 246, 63, 62, 97, 109, 249, 96, 229, 158, 209, 194, 219, 50, 53, 208, 121, 154, 147, 75,
        75, 95, 162, 136, 166, 172, 185, 222, 240, 56, 31, 18, 79, 64, 224, 155, 186, 136, 205, 172,
        180, 160, 66, 134, 123, 185, 45, 20, 203, 36, 111, 39, 249, 207, 207, 211, 174, 49, 9
    };

    TEST_ASSERT_EQUAL(BCC_RESULT_SUCCESS, status);

    TEST_ASSERT_EQUAL(sizeof(actual_txaux), size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(actual_txaux, bytes, sizeof(actual_txaux));

    bcc_signed_transaction_serialized_delete(bytes, size);
    bcc_transaction_delete(tx);
    bcc_transaction_finalized_delete(tf);
    bcc_transaction_signed_delete(txaux);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_add_input_returns_success_with_valid_value);
    RUN_TEST(test_add_input_returns_error_with_big_value);
    RUN_TEST(test_add_witness_returns_error_with_less_inputs);
    RUN_TEST(test_builder_finalize_error_code_no_inputs);
    RUN_TEST(test_builder_finalize_error_code_no_outputs);
    RUN_TEST(test_transaction_finalized_output_error_code_signature_mismatch);
    RUN_TEST(test_transaction_finalized_output_success);
    RUN_TEST(test_transaction_balance_zero);
    RUN_TEST(test_transaction_balance_negative);
    RUN_TEST(test_transaction_balance_positive);
    RUN_TEST(test_transaction_builder_balance_too_big);
    RUN_TEST(test_transaction_balance_without_fee_zero);
    RUN_TEST(test_transaction_balance_without_fee_negative);
    RUN_TEST(test_transaction_balance_without_fee_positive);
    RUN_TEST(test_transaction_builder_balance_without_fee_too_big);
    RUN_TEST(test_transaction_get_input_total);
    RUN_TEST(test_transaction_get_input_total_no_inputs);
    RUN_TEST(test_transaction_get_output_total);
    RUN_TEST(test_transaction_get_output_total_no_outputs);
    RUN_TEST(test_transaction_get_input_total_too_big);
    RUN_TEST(test_transaction_get_output_total_too_big);
    RUN_TEST(test_transaction_finalized_serialize);
    return UNITY_END();
}
