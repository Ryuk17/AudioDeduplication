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
    David Starkweather - dstarkweather@phash.org

*/


#include <getopt.h>
#include <stdio.h>
#include <cstdio>
#include "audiophash.h"


#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"




float *wav_read_float32(char *filename, uint32_t *sample_rate, uint64_t *total_sample_count) {
    unsigned int channels;
    float *buffer = drwav_open_file_and_read_pcm_frames_f32(filename, &channels, sample_rate, total_sample_count, NULL);
    if (buffer == nullptr) {
        printf("读取wav文件失败.");
    }
    //仅仅处理单通道音频
    if (channels != 1) {
        drwav_free(buffer, NULL);
        buffer = nullptr;
        *sample_rate = 0;
        *total_sample_count = 0;
    }
    return buffer;
}

void save_hash(char* save_path, uint32_t *hash, int nb_frames)
{
	FILE *file = fopen(save_path, "w");
	for(int i=0; i<nb_frames; i++)
	{
		fprintf(file, "%lu\n", *(hash+i));
	}
	fclose(file);
}

int main(int argc, char **argv) {

    char *file1 = "sample/ref.wav";
	char *file2 = "sample/src.wav";
    float threshold = 0.20;
    int block_size = 256;

    printf("file1: %s\n", file1);
    printf("file2: %s\n", file2);
    printf("threshold: %f\n", threshold);
    printf("blocksize: %d\n", block_size);

    uint32_t ref_sample_rate;
    uint64_t ref_total_sample_count;
    float *buf = wav_read_float32(file1, &ref_sample_rate, &ref_total_sample_count);

    if (!buf) {
        fprintf(stderr, " cannot read file %s, no such file\n", file1);
        return -1;
    }

    uint32_t src_sample_rate;
    uint64_t src_total_sample_count;
    float *buf2 = wav_read_float32(file2, &src_sample_rate, &src_total_sample_count);

    if (!buf2) {
        fprintf(stderr, " cannot read file %s, no such file", file2);
        return -1;
    }

    int nb_frames = 0;
    uint32_t *hash = ph_audiohash(buf, ref_total_sample_count, ref_sample_rate, nb_frames);
    if (!hash) {
        fprintf(stderr, "unable to calculate hash for %s\n", file1);
        return -1;
    }
    save_hash("ref.hash", hash, nb_frames);

    int nb_frames2 = 0;
    uint32_t *hash2 = ph_audiohash(buf2, src_total_sample_count, src_sample_rate, nb_frames2);

    if (!hash2) {
        fprintf(stderr, "unable to calculate hash for %s\n", file2);
        return -1;
    }
    save_hash("src.hash", hash2, nb_frames2);

    printf("nb_frames = %d, nb_frames2 = %d\n", nb_frames, nb_frames2);
    int Nc = 0;
    double maxC = 0;
    double *C = ph_audio_distance_ber(hash, nb_frames, hash2, nb_frames2,
                                      threshold, block_size, Nc);

    printf("Nc = %d, ", Nc);

    for (int i = 0; i < Nc; i++) {
        printf("C[i] = %f\n", C[i]);
        if (C[i] > maxC) maxC = C[i];
    }
    printf("cs = %f\n", maxC);

    free(buf);
    free(buf2);
    free(hash);
    free(hash2);
    free(C);

    return 0;
}
