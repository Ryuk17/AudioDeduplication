/*

    pHash, the open source perceptual hash library
    Copyright (C) 2009 Aetilius, Inc.
    All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Evan Klinger - eklinger@phash.org
    D Grant Starkweather - dstarkweather@phash.org

*/

#include "pHash.h"

#include <cmath>

const char phash_project[] = "%s. Copyright 2008-2010 Aetilius, Inc.";
char phash_version[255] = {0};
const char *ph_about() {
    if (phash_version[0] != 0) return phash_version;

    snprintf(phash_version, sizeof(phash_version), phash_project);
    return phash_version;
}


int ph_hamming_distance(const ulong64 hash1, const ulong64 hash2) {
    ulong64 x = hash1 ^ hash2;
    return __builtin_popcountll(x);
}




int ph_bitcount8(uint8_t val) {
    int num = 0;
    while (val) {
        ++num;
        val &= val - 1;
    }
    return num;
}

double ph_hammingdistance2(uint8_t *hashA, int lenA, uint8_t *hashB, int lenB) {
    if (lenA != lenB) {
        return -1.0;
    }
    if ((hashA == NULL) || (hashB == NULL) || (lenA <= 0)) {
        return -1.0;
    }
    double dist = 0;
    uint8_t D = 0;
    for (int i = 0; i < lenA; i++) {
        D = hashA[i] ^ hashB[i];
        dist = dist + (double)ph_bitcount8(D);
    }
    double bits = (double)lenA * 8;
    return dist / bits;
}

TxtHashPoint *ph_texthash(const char *filename, int *nbpoints) {
    int count;
    TxtHashPoint *TxtHash = NULL;
    TxtHashPoint WinHash[WindowLength];
    char kgram[KgramLength];

    FILE *pfile = fopen(filename, "r");
    if (!pfile) {
        return NULL;
    }
    struct stat fileinfo;
    fstat(fileno(pfile), &fileinfo);
    count = fileinfo.st_size - WindowLength + 1;
    count = (int)(0.01 * count);
    int d;
    ulong64 hashword = 0ULL;

    TxtHash = (TxtHashPoint *)malloc(count * sizeof(struct ph_hash_point));
    if (!TxtHash) {
        return NULL;
    }
    *nbpoints = 0;
    int i, first = 0, last = KgramLength - 1;
    int text_index = 0;
    int win_index = 0;
    for (i = 0; i < KgramLength; i++) { /* calc first kgram */
        d = fgetc(pfile);
        if (d == EOF) {
            free(TxtHash);
            return NULL;
        }
        if (d <= 47) /*skip cntrl chars*/
            continue;
        if (((d >= 58) && (d <= 64)) || ((d >= 91) && (d <= 96)) ||
            (d >= 123)) /*skip punct*/
            continue;
        if ((d >= 65) && (d <= 90)) /*convert upper to lower case */
            d = d + 32;

        kgram[i] = (char)d;
        hashword = hashword << delta;      /* rotate left or shift left ??? */
        hashword = hashword ^ textkeys[d]; /* right now, rotate breaks it */
    }

    WinHash[win_index].hash = hashword;
    WinHash[win_index++].index = text_index;
    struct ph_hash_point minhash;
    minhash.hash = ULLONG_MAX;
    minhash.index = 0;
    struct ph_hash_point prev_minhash;
    prev_minhash.hash = ULLONG_MAX;
    prev_minhash.index = 0;

    while ((d = fgetc(pfile)) != EOF) { /*remaining kgrams */
        text_index++;
        if (d == EOF) {
            free(TxtHash);
            return NULL;
        }
        if (d <= 47) /*skip cntrl chars*/
            continue;
        if (((d >= 58) && (d <= 64)) || ((d >= 91) && (d <= 96)) ||
            (d >= 123)) /*skip punct*/
            continue;
        if ((d >= 65) && (d <= 90)) /*convert upper to lower case */
            d = d + 32;

        ulong64 oldsym = textkeys[kgram[first % KgramLength]];

        /* rotate or left shift ??? */
        /* right now, rotate breaks it */
        oldsym = oldsym << delta * KgramLength;
        hashword = hashword << delta;
        hashword = hashword ^ textkeys[d];
        hashword = hashword ^ oldsym;
        kgram[last % KgramLength] = (char)d;
        first++;
        last++;

        WinHash[win_index % WindowLength].hash = hashword;
        WinHash[win_index % WindowLength].index = text_index;
        win_index++;

        if (win_index >= WindowLength) {
            minhash.hash = ULLONG_MAX;
            for (i = win_index; i < win_index + WindowLength; i++) {
                if (WinHash[i % WindowLength].hash <= minhash.hash) {
                    minhash.hash = WinHash[i % WindowLength].hash;
                    minhash.index = WinHash[i % WindowLength].index;
                }
            }
            if (minhash.hash != prev_minhash.hash) {
                TxtHash[(*nbpoints)].hash = minhash.hash;
                TxtHash[(*nbpoints)++].index = minhash.index;
                prev_minhash.hash = minhash.hash;
                prev_minhash.index = minhash.index;

            } else {
                TxtHash[*nbpoints].hash = prev_minhash.hash;
                TxtHash[(*nbpoints)++].index = prev_minhash.index;
            }
            win_index = 0;
        }
    }

    fclose(pfile);
    return TxtHash;
}

TxtMatch *ph_compare_text_hashes(TxtHashPoint *hash1, int N1,
                                 TxtHashPoint *hash2, int N2, int *nbmatches) {
    int max_matches = (N1 >= N2) ? N1 : N2;
    TxtMatch *found_matches =
        (TxtMatch *)malloc(max_matches * sizeof(TxtMatch));
    if (!found_matches) {
        return NULL;
    }

    *nbmatches = 0;
    int i, j;
    for (i = 0; i < N1; i++) {
        for (j = 0; j < N2; j++) {
            if (hash1[i].hash == hash2[j].hash) {
                int m = i + 1;
                int n = j + 1;
                int cnt = 1;
                while ((m < N1) && (n < N2) &&
                       (hash1[m++].hash == hash2[n++].hash)) {
                    cnt++;
                }
                found_matches[*nbmatches].first_index = i;
                found_matches[*nbmatches].second_index = j;
                found_matches[*nbmatches].length = cnt;
                (*nbmatches)++;
            }
        }
    }
    return found_matches;
}
