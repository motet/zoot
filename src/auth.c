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

#include <zoot/suit.h>

int suit_env_unwrap(const uint8_t * pem, 
        const uint8_t * env, const size_t len_env,
        const uint8_t ** man, size_t * len_man)
{
    /* initialize COSE Sign1 context for authentication wrapper */
    cose_sign_context_t ctx;
    if (cose_sign_init(&ctx, cose_mode_r, pem)) return 1;

    /* seek to beginning of authentication wrapper */
    uint8_t * auth;
    size_t len_auth;
    nanocbor_value_t nc, map, arr;
    nanocbor_decoder_init(&nc, env, len_env);
    if (nanocbor_enter_map(&nc, &map) < 0) return 1;
    while (!nanocbor_at_end(&map)) {
        int32_t map_key;
        if (nanocbor_get_int32(&map, &map_key) < 0) return 1;
        if (map_key == suit_env_auth_wrapper) {
            if (nanocbor_get_bstr(&map, (const uint8_t **) &auth, &len_auth) < 0) 
                return 1;
            else break;
        }
        nanocbor_skip(&map);
    }
    nanocbor_decoder_init(&nc, auth, len_auth);
    if (nanocbor_enter_array(&nc, &arr) < 0) return 1;

    /* verify signature on authentication wrapper and get payload */ 
    uint8_t * pld;
    size_t len_pld;
    if (cose_sign1_read(&ctx, arr.cur, arr.end - arr.cur, 
                (const uint8_t **) &pld, &len_pld))
        return 1;

    /* extract the manifest hash */
    uint8_t * hash;
    size_t len_hash;
    nanocbor_decoder_init(&nc, pld, len_pld);
    if (nanocbor_enter_array(&nc, &arr) < 0) return 1;
    nanocbor_skip(&arr);
    nanocbor_get_bstr(&arr, (const uint8_t **) &hash, &len_hash);

    /* extract manifest */
    while (!nanocbor_at_end(&map)) {
        int32_t map_key;
        if (nanocbor_get_int32(&map, &map_key) < 0) return 1;
        if (map_key == suit_env_man) {
            if (nanocbor_get_bstr(&map, man, len_man) < 0) 
                return 1;
            else break;
        }
        nanocbor_skip(&map);
    }

    /* hash the manifest and write it to the end of the output buffer */
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    const mbedtls_md_info_t * md_info = mbedtls_md_info_from_type(md_type);
    size_t md_size = mbedtls_md_get_size(md_info);
    uint8_t hash_out[md_size];
    mbedtls_md(md_info, *man, *len_man, hash_out);
    if (memcmp(hash, hash_out, md_size)) return 1; 

    /* clean up */
    cose_sign_free(&ctx);
    return 0;
}

int suit_env_wrap(const uint8_t * pem,
        const uint8_t * man, const size_t len_man,
        uint8_t * env, size_t * len_env)
{
    /* initialize COSE Sign1 context for authentication wrapper */
    cose_sign_context_t ctx;
    if (cose_sign_init(&ctx, cose_mode_w, pem)) return 1;

    /* hash the manifest and write it to the end of the output buffer */
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    const mbedtls_md_info_t * md_info = mbedtls_md_info_from_type(md_type);
    size_t md_size = mbedtls_md_get_size(md_info);
    mbedtls_md(md_info, man, len_man, env + *len_env - md_size);

    /* serialize the authentication wrapper payload in place */
    nanocbor_encoder_t nc;
    nanocbor_encoder_init(&nc, env + *len_env - md_size - 4, 4);
    nanocbor_fmt_array(&nc, 2);
    nanocbor_fmt_uint(&nc, suit_md_alg_sha256);
    nanocbor_fmt_bstr(&nc, md_size);

    /* write the authentication wrapper */
    size_t len_auth = *len_env - 5;
    cose_sign1_write(&ctx, env + *len_env - md_size - 4, md_size + 4, env + 5, &len_auth);

    /* encode the envelope header */
    nanocbor_encoder_init(&nc, env, 5);
    nanocbor_fmt_map(&nc, 2);
    nanocbor_fmt_uint(&nc, suit_env_auth_wrapper);
    nanocbor_fmt_bstr(&nc, len_auth + 1);  
    nanocbor_fmt_array(&nc, 1);

    /* skip to end of authentication wrapper and encode the manifest */
    nanocbor_encoder_init(&nc, env + 5 + len_auth, *len_env - 5 - len_auth);
    nanocbor_fmt_uint(&nc, suit_env_man);
    nanocbor_fmt_bstr(&nc, len_man);
    *len_env = len_auth + 5 + nanocbor_encoded_len(&nc);
    memcpy(env + *len_env, man, len_man);
    *len_env += len_man;

    /* clean up */
    cose_sign_free(&ctx);
    return 0;
    
}