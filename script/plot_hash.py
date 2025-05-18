# encoding: utf-8
"""
@author:  Ryuk
@contact: jeryuklau@gmail.com
"""

import numpy as np
import matplotlib.pyplot as plt
from typing import List


def int_to_bits_array(n: int, bits: int = 32) -> np.ndarray:
    return np.array([(n >> i) & 1 for i in reversed(range(bits))], dtype=np.uint8)


def read_hashes_from_file(filename: str) -> List[int]:
    hashes = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            # 自动判断是十进制还是十六进制
            try:
                if line.startswith('0x') or line.startswith('0X'):
                    hashes.append(int(line, 16))
                else:
                    hashes.append(int(line))
            except ValueError:
                print(f"⚠️ 无法解析行: {line}")
    return hashes


def hashes_to_bit_image(hash_list: List[int]) -> np.ndarray:
    return np.array([int_to_bits_array(h) for h in hash_list], dtype=np.uint8).T


def plot_bit_image(bit_image: np.ndarray):
    plt.figure(figsize=(10, 4))  # 控制整体图像大小
    plt.imshow(bit_image, cmap='gray', vmin=0, vmax=1, aspect='auto')
    plt.title(f'Bit Visualization ({bit_image.shape[0]} x {bit_image.shape[1]})')
    plt.xlabel('Hash Index')
    plt.ylabel('Bit Index')
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    # 主程序
    filename = 'src.hash'  # 修改为你的文件名路径
    hash_list = read_hashes_from_file(filename)
    bit_image = hashes_to_bit_image(hash_list)
    plot_bit_image(bit_image)



