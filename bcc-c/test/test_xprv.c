#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../bcc.h"
#include "unity/unity.h"

void can_serialize_xprv(void)
{
    uint8_t bytes[XPRV_SIZE] = {0};
    bytes[0] = 0b00000000;
    bytes[31] = 0b01000000;

    bcc_xprv *xprv;
    bcc_result rc = bcc_xprv_from_bytes(bytes, &xprv);

    uint8_t *new_bytes = bcc_xprv_to_bytes(xprv);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(bytes, new_bytes, XPRV_SIZE);
    bcc_xprv_bytes_delete(new_bytes);
    bcc_xprv_delete(xprv);
}

void xprv_from_invalid_bytes_returns_failure()
{
    uint8_t bytes[XPRV_SIZE] = {0};
    bcc_xprv *xprv;
    bcc_result rc = bcc_xprv_from_bytes(bytes, &xprv);
    TEST_ASSERT_EQUAL(1, rc);
}

void xprv_from_valid_bytes_returns_success()
{
    uint8_t bytes[XPRV_SIZE] = {0};
    bytes[0] = 0b00000000;
    bytes[31] = 0b01000000;

    bcc_xprv *xprv;
    bcc_result rc = bcc_xprv_from_bytes(bytes, &xprv);

    TEST_ASSERT_EQUAL(0, rc);
    bcc_xprv_delete(xprv);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(can_serialize_xprv);
    RUN_TEST(xprv_from_invalid_bytes_returns_failure);
    RUN_TEST(xprv_from_valid_bytes_returns_success);
    return UNITY_END();
}
