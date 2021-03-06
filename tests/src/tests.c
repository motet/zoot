/*
 * Copyright 2020 RISE Research Institutes of Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <ztest.h>
#include <zoot/suit.h>
#include "vectors.h"

static size_t test_size = 34768;
static uint8_t test_digest[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};
static uint8_t test_vendor_id[] = {
    0xfa, 0x6b, 0x4a, 0x53, 0xd5, 0xad, 0x5f, 0xdf,
    0xbe, 0x9d, 0xe6, 0x63, 0xe4, 0xd4, 0x1f, 0xfe
};
static uint8_t test_class_id[] = {
    0x14, 0x92, 0xaf, 0x14, 0x25, 0x69, 0x5e, 0x48,
    0xbf, 0x42, 0x9b, 0x2d, 0x51, 0xf2, 0xab, 0x45
};
static uint8_t test_uri[] = {
    'h', 't', 't', 'p', ':', '/', '/', 'e', 
    'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 
    'o', 'm', '/', 'f', 'i', 'l', 'e', '.', 
    'b', 'i', 'n'
};

#define SUIT_TEST_PARSE(x)                                              \
    size_t len_man = strlen(SUIT_MANIFEST_##x) / 2;                     \
    uint8_t man[len_man];                                               \
    _xxd_r(SUIT_MANIFEST_##x, man);                                     \
    suit_context_t ctx;                                                 \
    zassert_false(suit_parse_init(&ctx, man, len_man),                  \
            "Failed to parse SUIT manifest.");

/* converts hex-formatted IETF examples to raw bytes */
void _xxd_r(char * hex, uint8_t * out)
{
    size_t len_hex = strlen(hex);
    uint8_t buf[5] = {'0', 'x', 0, 0, 0};
    for (int i = 0; i < len_hex / 2; i++) {
        buf[2] = hex[i]; buf[3] = hex[i+1];
        out[i] = strtol(buf, NULL, 0);
        hex++;
    } 
}

const uint8_t * pem_pub = SUIT_TEST_KEY_256_PUB;
const uint8_t * pem_prv = SUIT_TEST_KEY_256_PRV;

void test_suit_boot(void) {
    SUIT_TEST_PARSE(0);

    /* encode a signed manifest envelope */
    size_t len_env = 256; uint8_t env[len_env];
    zassert_false(suit_manifest_wrap(
                pem_prv, man, len_man, env, &len_env),
                "Failed to write manifest envelope.");

    /* verify encoded envelope and extract manifest */
    uint8_t * man_out;
    size_t len_man_out;
    zassert_false(suit_manifest_unwrap(
                pem_pub, env, len_env,
                (const uint8_t **) &man_out, &len_man_out),
                "Failed to authenticate envelope contents.");

    /* check that extracted manifest matches the original */
    zassert_true(len_man == len_man_out,
            "Failed to extract manifest.");
    zassert_false(memcmp(man, man_out, len_man),
            "Failed to extract manifest.");
}
 
void test_suit_download_install(void) {
    SUIT_TEST_PARSE(1);
    zassert_true(
            suit_class_id_is_match(&ctx, 0, test_class_id, sizeof(test_class_id)),
            "Class ID mismatch.");
    zassert_true(
            suit_vendor_id_is_match(&ctx, 0, test_vendor_id, sizeof(test_vendor_id)),
            "Vendor ID mismatch.");
    zassert_true(
            suit_digest_is_match(&ctx, 0, test_digest, sizeof(test_digest)),
            "Image digest mismatch.");

    uint8_t * uri; size_t len_uri;
    suit_get_uri(&ctx, 0, (const uint8_t **) &uri, &len_uri);
    zassert_false(memcmp(test_uri, uri, len_uri), "Unexpected URI.");

    zassert_true(suit_get_size(&ctx, 0) == test_size, "Unexpeted image size.");
    zassert_false(suit_must_run(&ctx, 0), "Unexpected run directive.");
}

void test_suit_download_install_boot(void) {
    SUIT_TEST_PARSE(2);
    zassert_true(
            suit_class_id_is_match(&ctx, 0, test_class_id, sizeof(test_class_id)),
            "Class ID mismatch.");
    zassert_true(
            suit_vendor_id_is_match(&ctx, 0, test_vendor_id, sizeof(test_vendor_id)),
            "Vendor ID mismatch.");
    zassert_true(
            suit_digest_is_match(&ctx, 0, test_digest, sizeof(test_digest)),
            "Image digest mismatch.");

    uint8_t * uri; size_t len_uri;
    suit_get_uri(&ctx, 0, (const uint8_t **) &uri, &len_uri);
    zassert_false(memcmp(test_uri, uri, len_uri), "Unexpected URI.");

    zassert_true(suit_get_size(&ctx, 0) == test_size, "Unexpeted image size.");
    zassert_true(suit_must_run(&ctx, 0), "Unexpected run directive.");
}

void test_suit_load_external_storage(void) {
    SUIT_TEST_PARSE(3);
    zassert_true(
            suit_class_id_is_match(&ctx, 0, test_class_id, sizeof(test_class_id)),
            "Class ID mismatch.");
    zassert_true(
            suit_vendor_id_is_match(&ctx, 0, test_vendor_id, sizeof(test_vendor_id)),
            "Vendor ID mismatch.");
    zassert_true(
            suit_digest_is_match(&ctx, 0, test_digest, sizeof(test_digest)),
            "Image digest mismatch.");
    zassert_true(
            suit_get_source_component(&ctx, 1) == &ctx.components[0],
            "Unexpected source component.");

    uint8_t * uri; size_t len_uri;
    suit_get_uri(&ctx, 0, (const uint8_t **) &uri, &len_uri);
    zassert_false(memcmp(test_uri, uri, len_uri), "Unexpected URI.");

    zassert_true(suit_get_size(&ctx, 0) == test_size, "Unexpeted image size.");
    zassert_false(suit_must_run(&ctx, 0), "Unexpected run directive.");
    zassert_true(suit_must_run(&ctx, 1), "Unexpected run directive.");
}

void test_suit_load_decompress_external_storage(void) {
    SUIT_TEST_PARSE(4);
    zassert_true(
            suit_class_id_is_match(&ctx, 0, test_class_id, sizeof(test_class_id)),
            "Class ID mismatch.");
    zassert_true(
            suit_vendor_id_is_match(&ctx, 0, test_vendor_id, sizeof(test_vendor_id)),
            "Vendor ID mismatch.");
    zassert_true(
            suit_digest_is_match(&ctx, 0, test_digest, sizeof(test_digest)),
            "Image digest mismatch.");
    zassert_true(
            suit_get_source_component(&ctx, 1) == &ctx.components[0],
            "Unexpected source component.");

    uint8_t * uri; size_t len_uri;
    suit_get_uri(&ctx, 0, (const uint8_t **) &uri, &len_uri);
    zassert_false(memcmp(test_uri, uri, len_uri), "Unexpected URI.");

    zassert_true(suit_get_size(&ctx, 0) == test_size, "Unexpeted image size.");
    zassert_false(suit_must_run(&ctx, 0), "Unexpected run directive.");
    zassert_true(suit_must_run(&ctx, 1), "Unexpected run directive.");
}

void test_suit_compatibility_download_install_boot(void) {
    SUIT_TEST_PARSE(5);
    zassert_true(
            suit_class_id_is_match(&ctx, 1, test_class_id, sizeof(test_class_id)),
            "Class ID mismatch.");
    zassert_true(
            suit_vendor_id_is_match(&ctx, 1, test_vendor_id, sizeof(test_vendor_id)),
            "Vendor ID mismatch.");
    zassert_true(
            suit_digest_is_match(&ctx, 1, test_digest, sizeof(test_digest)),
            "Image digest mismatch.");
    zassert_true(
            suit_get_source_component(&ctx, 1) == &ctx.components[0],
            "Unexpected source component.");

    uint8_t * uri; size_t len_uri;
    suit_get_uri(&ctx, 0, (const uint8_t **) &uri, &len_uri);
    zassert_false(memcmp(test_uri, uri, len_uri), "Unexpected URI.");

    zassert_true(suit_get_size(&ctx, 1) == test_size, "Unexpeted image size.");
    zassert_false(suit_must_run(&ctx, 0), "Unexpected run directive.");
    zassert_true(suit_must_run(&ctx, 1), "Unexpected run directive.");
}

void test_suit_two_images(void) {
    SUIT_TEST_PARSE(6);
    zassert_true(
            suit_class_id_is_match(&ctx, 0, test_class_id, sizeof(test_class_id)),
            "Class ID mismatch.");
    zassert_true(
            suit_vendor_id_is_match(&ctx, 0, test_vendor_id, sizeof(test_vendor_id)),
            "Vendor ID mismatch.");
    zassert_true(
            suit_digest_is_match(&ctx, 0, test_digest, sizeof(test_digest)),
            "Image digest mismatch.");

    zassert_true(suit_get_size(&ctx, 0) == test_size, "Unexpeted image size.");
    zassert_false(suit_must_run(&ctx, 0), "Unexpected run directive.");

    uint8_t test_uri_[] = {
        'h', 't', 't', 'p', ':', '/', '/', 'e', 
        'x', 'a', 'm', 'p', 'l', 'e', '.', 'c', 
        'o', 'm', '/', 'f', 'i', 'l', 'e', '1',
        '.', 'b', 'i', 'n'
    };
    
    uint8_t * uri; size_t len_uri;
    suit_get_uri(&ctx, 0, (const uint8_t **) &uri, &len_uri);
    zassert_false(memcmp(test_uri_, uri, len_uri), "Unexpected URI.");
}
