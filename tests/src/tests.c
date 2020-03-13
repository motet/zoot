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

#define SUIT_MANIFEST(x)                                                \
    size_t len_man = strlen(SUIT_MANIFEST_##x) / 2;                     \
    uint8_t man[len_man];                                               \
    _xxd_r(SUIT_MANIFEST_##x, man);

#define SUIT_TEST_WRAP(x)                                               \
    size_t len_env = x;                                                 \
    uint8_t env[x];                                                     \
    zassert_false(suit_env_wrap(pem_prv, man, len_man, env, &len_env),  \
                "Failed to write manifest envelope.");

#define SUIT_TEST_UNWRAP                                                \
    uint8_t * man_out;                                                  \
    size_t len_man_out;                                                 \
    zassert_false(suit_env_unwrap(                                      \
                pem_pub, env, len_env,                                  \
                (const uint8_t **) &man_out, &len_man_out),             \
                "Failed to authenticate envelope contents.");           \
    zassert_true(len_man == len_man_out,                                \
            "Failed to extract manifest.");                             \
    zassert_false(memcmp(man, man_out, len_man),                        \
            "Failed to extract manifest.");

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
    SUIT_MANIFEST(0)
    SUIT_TEST_WRAP(512)
    SUIT_TEST_UNWRAP
}
 
void test_suit_download_install(void) {
    SUIT_MANIFEST(1)
    SUIT_TEST_WRAP(512)
    SUIT_TEST_UNWRAP
}

void test_suit_download_install_boot(void) {
    SUIT_MANIFEST(2)
    SUIT_TEST_WRAP(512)
    SUIT_TEST_UNWRAP
}
