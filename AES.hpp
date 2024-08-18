#if !defined(AES_ED_FUNCTION_HPP)
#define AES_ED_FUNCTION_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <stdexcept>

// S-box and inverse S-box (example values, please use actual AES S-box values)
constexpr std::array<uint8_t, 256> sbox = {
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
	0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
	0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
	0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
	0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
	0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
	0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
	0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
	0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
	0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
	0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
	0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
	0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
	0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
	0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
	0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
	0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};
constexpr std::array<uint8_t, 256> inv_sbox = {
	0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38,
	0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
	0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
	0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
	0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d,
	0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
	0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2,
	0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
	0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
	0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
	0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda,
	0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
	0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a,
	0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
	0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
	0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
	0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea,
	0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
	0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85,
	0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
	0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
	0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
	0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20,
	0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
	0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31,
	0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
	0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
	0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
	0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0,
	0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
	0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26,
	0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

// AES constants
constexpr int Nb = 4;  // Number of 32-bit words in a block (4 for AES)
constexpr int Nk = 8;  // Number of 32-bit words in the key (4/6/8 for AES-128/192/256)
constexpr int Nr = 14; // Number of rounds (10/12/14 for AES-128/192/256)

//xtimes function values
constexpr std::array<uint32_t, 11> RCON = {
	0x01, // 第一个常量
	0x02, // 第二个常量
	0x04, // 第三个常量
	0x08, // 第四个常量
	0x10, // 第五个常量
	0x20, // 第六个常量
	0x40, // 第七个常量
	0x80, // 第八个常量
	0x1B, // 第九个常量
	0x36, // 第十个常量
	0x6C  // 仅限AES-256（14轮）
};

class AES {
public:
	AES(const std::vector<uint32_t>& key) {
		// 在构造函数中将32比特密钥转换为8比特
		std::vector<uint8_t> key_bytes = FromUint32(key);
		keySchedule = KeyExpansion(key_bytes);
	}

	std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext) {
		// 确保输入数据是 16 字节的倍数
		if (plaintext.size() % 16 != 0) {
			throw std::invalid_argument("Plaintext length must be a multiple of 16 bytes.");
		}
		
		std::vector<uint8_t> ciphertext;
		size_t num_blocks = plaintext.size() / 16;

		// 处理所有完整的 16 字节块
		for (size_t i = 0; i < num_blocks; ++i) {
			std::vector<uint8_t> state(16);
			for (size_t j = 0; j < 16; ++j) {
				state[j] = plaintext[i * 16 + j];
			}
			AddRoundKey(state, 0);

			for (int round = 1; round < Nr; ++round) {
				SubBytes(state);
				ShiftRows(state);
				MixColumns(state);
				AddRoundKey(state, round);
			}

			SubBytes(state);
			ShiftRows(state);
			AddRoundKey(state, Nr);

			// 将加密后的块添加到密文中
			for (size_t j = 0; j < 16; ++j) {
				ciphertext.push_back(state[j]);
			}
		}

		return ciphertext;
	}

	std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext) {
		// 确保输入数据是 16 字节的倍数
		if (ciphertext.size() % 16 != 0) {
			throw std::invalid_argument("Ciphertext length must be a multiple of 16 bytes.");
		}
		
		std::vector<uint8_t> plaintext;
		size_t num_blocks = ciphertext.size() / 16;

		// 处理所有完整的 16 字节块
		for (size_t i = 0; i < num_blocks; ++i) {
			std::vector<uint8_t> state(16);
			for (size_t j = 0; j < 16; ++j) {
				state[j] = ciphertext[i * 16 + j];
			}
			AddRoundKey(state, Nr);

			for (int round = Nr - 1; round > 0; --round) {
				InvShiftRows(state);
				InvSubBytes(state);
				AddRoundKey(state, round);
				InvMixColumns(state);
			}

			InvShiftRows(state);
			InvSubBytes(state);
			AddRoundKey(state, 0);

			// 将解密后的块添加到明文中
			for (size_t j = 0; j < 16; ++j) {
				plaintext.push_back(state[j]);
			}
		}

		return plaintext;
	}

	// 新增的uint32_t接口
	std::vector<uint32_t> encrypt_uint32(const std::vector<uint32_t>& plaintext) {
		std::vector<uint8_t> byte_plaintext = FromUint32(plaintext);

		// 记录开始时间
		start_time = std::chrono::high_resolution_clock::now();

		std::vector<uint8_t> encrypted_bytes = encrypt(byte_plaintext);

		// 记录结束时间
		end_time = std::chrono::high_resolution_clock::now();

		return ToUint32(encrypted_bytes);
	}

	std::vector<uint32_t> decrypt_uint32(const std::vector<uint32_t>& ciphertext) {
		std::vector<uint8_t> byte_ciphertext = FromUint32(ciphertext);

		// 记录开始时间
		start_time = std::chrono::high_resolution_clock::now();

		std::vector<uint8_t> decrypted_bytes = decrypt(byte_ciphertext);

		// 记录结束时间
		end_time = std::chrono::high_resolution_clock::now();

		return ToUint32(decrypted_bytes);
	}

	// 公共成员变量，用于记录开始和结束时间
	std::chrono::high_resolution_clock::time_point start_time;
	std::chrono::high_resolution_clock::time_point end_time;

private:
	std::vector<uint8_t> keySchedule;

	// 新增的FromUint32和ToUint32转换方法
	std::vector<uint8_t> FromUint32(const std::vector<uint32_t>& input) {
		std::vector<uint8_t> output(input.size() * 4);
		for (size_t i = 0; i < input.size(); ++i) {
			output[i * 4] = static_cast<uint8_t>(input[i] >> 24);
			output[i * 4 + 1] = static_cast<uint8_t>(input[i] >> 16);
			output[i * 4 + 2] = static_cast<uint8_t>(input[i] >> 8);
			output[i * 4 + 3] = static_cast<uint8_t>(input[i]);
		}
		return output;
	}

	std::vector<uint32_t> ToUint32(const std::vector<uint8_t>& input) {
		std::vector<uint32_t> output(input.size() / 4);
		for (size_t i = 0; i < output.size(); ++i) {
			output[i] = (static_cast<uint32_t>(input[i * 4]) << 24) |
				(static_cast<uint32_t>(input[i * 4 + 1]) << 16) |
				(static_cast<uint32_t>(input[i * 4 + 2]) << 8) |
				static_cast<uint32_t>(input[i * 4 + 3]);
		}
		return output;
	}

	void AddRoundKey(std::vector<uint8_t>& state, int round) {
		for (int i = 0; i < Nb * 4; ++i) {
			state[i] ^= static_cast<uint8_t>(keySchedule[round * Nb + i / 4] >> ((3 - (i % 4)) * 8));
		}
	}

	void SubBytes(std::vector<uint8_t>& state) {
		for (int i = 0; i < Nb * 4; ++i) {
			state[i] = sbox[state[i]];
		}
	}

	void InvSubBytes(std::vector<uint8_t>& state) {
		for (int i = 0; i < Nb * 4; ++i) {
			state[i] = inv_sbox[state[i]];
		}
	}

	void ShiftRows(std::vector<uint8_t>& state) {
		std::vector<uint8_t> temp(4 * Nb);
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < Nb; ++j) {
				temp[i + 4 * j] = state[i + 4 * ((j + i) % Nb)];
			}
		}
		state = temp;
	}

	void InvShiftRows(std::vector<uint8_t>& state) {
		std::vector<uint8_t> temp(4 * Nb);
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < Nb; ++j) {
				temp[i + 4 * j] = state[i + 4 * ((j - i + Nb) % Nb)];
			}
		}
		state = temp;
	}

	void MixColumns(std::vector<uint8_t>& state) {
		for (int i = 0; i < Nb; ++i) {
			uint8_t a[4];
			uint8_t b[4];
			for (int j = 0; j < 4; ++j) {
				a[j] = state[i * 4 + j];
				b[j] = (state[i * 4 + j] & 0x80) ? (state[i * 4 + j] << 1) ^ 0x1b : (state[i * 4 + j] << 1);
			}
			state[i * 4] = b[0] ^ a[3] ^ a[2] ^ b[1] ^ a[1];
			state[i * 4 + 1] = b[1] ^ a[0] ^ a[3] ^ b[2] ^ a[2];
			state[i * 4 + 2] = b[2] ^ a[1] ^ a[0] ^ b[3] ^ a[3];
			state[i * 4 + 3] = b[3] ^ a[2] ^ a[1] ^ b[0] ^ a[0];
		}
	}

	void InvMixColumns(std::vector<uint8_t>& state) {
		for (int i = 0; i < Nb; ++i) {
			uint8_t a[4];
			for (int j = 0; j < 4; ++j) {
				a[j] = state[i * 4 + j];
			}
			state[i * 4] = Multiply(a[0], 0x0e) ^ Multiply(a[1], 0x0b) ^ Multiply(a[2], 0x0d) ^ Multiply(a[3], 0x09);
			state[i * 4 + 1] = Multiply(a[1], 0x0e) ^ Multiply(a[2], 0x0b) ^ Multiply(a[3], 0x0d) ^ Multiply(a[0], 0x09);
			state[i * 4 + 2] = Multiply(a[2], 0x0e) ^ Multiply(a[3], 0x0b) ^ Multiply(a[0], 0x0d) ^ Multiply(a[1], 0x09);
			state[i * 4 + 3] = Multiply(a[3], 0x0e) ^ Multiply(a[0], 0x0b) ^ Multiply(a[1], 0x0d) ^ Multiply(a[2], 0x09);
		}
	}

	uint8_t Multiply(uint8_t x, uint8_t y) {
		return ((y & 1) * x) ^
			((y >> 1 & 1) * xtime(x)) ^
			((y >> 2 & 1) * xtime(xtime(x))) ^
			((y >> 3 & 1) * xtime(xtime(xtime(x)))) ^
			((y >> 4 & 1) * xtime(xtime(xtime(xtime(x)))));
	}

	uint8_t xtime(uint8_t x) {
		return (x << 1) ^ ((x & 0x80) ? 0x1b : 0x00);
	}

	std::vector<uint8_t> RotWord(const std::vector<uint8_t>& word) {
		std::vector<uint8_t> result = { word[1], word[2], word[3], word[0] };
		return result;
	}

	std::vector<uint8_t> SubWord(const std::vector<uint8_t>& word) {
		std::vector<uint8_t> result(4);
		for (int i = 0; i < 4; ++i) {
			result[i] = sbox[word[i]];
		}
		return result;
	}

	std::vector<uint8_t> KeyExpansion(const std::vector<uint8_t>& key) {
		std::vector<uint8_t> expandedKey(Nb * 4 * (Nr + 1));

		for (int i = 0; i < Nk * 4; ++i) {
			expandedKey[i] = key[i];
		}

		std::vector<uint8_t> temp(4);
		for (int i = Nk; i < Nb * (Nr + 1); ++i) {
			for (int j = 0; j < 4; ++j) {
				temp[j] = expandedKey[(i - 1) * 4 + j];
			}
			if (i % Nk == 0) {
				temp = SubWord(RotWord(temp));
				temp[0] ^= RCON[i / Nk];
			}
			else if (Nk > 6 && i % Nk == 4) {
				temp = SubWord(temp);
			}
			for (int j = 0; j < 4; ++j) {
				expandedKey[i * 4 + j] = expandedKey[(i - Nk) * 4 + j] ^ temp[j];
			}
		}
		return expandedKey;
	}
};

#endif