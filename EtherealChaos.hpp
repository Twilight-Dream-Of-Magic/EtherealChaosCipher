#if !defined(ETHEREAL_CHAOS_ED_FUNCTION_HPP)
#define ETHEREAL_CHAOS_ED_FUNCTION_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <random>
#include <exception>

//艺术名字：灵动混沌加密（Ethereal Chaos Encryption）

//我们的加密算法采用了基于分块交换的并行ARX结构设计，称为 **Block Exchange Parallel ARX (BEP-ARX)**。
//官方名称为 **Confusion-Diffusion Parallel Encryption (CDPE)**，中文解释为 **混淆扩散并行加密算法**。

// 32位常量数组，用于轮函数中的常量
constexpr std::array<uint32_t, 16> ROUND_CONSTANTS =
{
	0x16B2C40B, 0xC117176A, 0x0F9A2598, 0xA1563ACA,
	0x243F6A88, 0x85A308D3, 0x13198102, 0xE0370734,
	0x9E3779B9, 0x7F4A7C15, 0xF39CC060, 0x5CEDC834,
	0xB7E15162, 0x8AED2A6A, 0xBF715880, 0x9CF4F3C7
};

class EtherealChaos
{
private:
	// 工程修复点：扁平化 SBOX_2D，避免 64 次堆分配
	// 逻辑不变：SBOX_2D 的“行数=sbox_count，列数=state_size”
	std::vector<std::uint32_t> sbox_flat;  // size = sbox_count * state_size

	std::vector<std::uint32_t> round_keys;
	std::size_t state_size = 16;   // must be multiple of 16
	std::size_t sbox_count = 64;

	static inline std::uint32_t rotl32(std::uint32_t v, std::uint32_t s) noexcept
	{
		s &= 31u;
		if (s == 0u) return v;
		return (v << s) | (v >> (32u - s));
	}

	static inline std::uint32_t rotr32(std::uint32_t v, std::uint32_t s) noexcept
	{
		s &= 31u;
		if (s == 0u) return v;
		return (v >> s) | (v << (32u - s));
	}

	inline std::uint32_t ARX_forward(std::uint32_t block, std::uint32_t key_part) noexcept
	{
		block = block + key_part;
		block = rotl32(block, key_part);
		block ^= key_part;
		return block;
	}

	inline std::uint32_t ARX_backward(std::uint32_t block, std::uint32_t key_part) noexcept
	{
		block ^= key_part;
		block = rotr32(block, key_part);
		block = block - key_part;
		return block;
	}

	// 工程修复点：不再每次切 vector，改为索引视图
	// 思路不改：“分两半 + forward/backward 交织 + 常量扰动”
	std::uint32_t complex_key_mixing(const std::vector<std::uint32_t>& master_key, std::size_t round_num)
	{
		const std::size_t n = master_key.size();
		if (n == 0) return 0;

		const std::size_t half = n / 2;
		const std::size_t a_len = (half == 0) ? 1 : half;
		const std::size_t b_len = (n - half == 0) ? 1 : (n - half);

		// 访问 A: master_key[0..half-1], B: master_key[half..n-1]
		auto A_at = [&](std::size_t i) -> std::uint32_t {
			return master_key[i % a_len];
		};
		auto B_at = [&](std::size_t i) -> std::uint32_t {
			return master_key[half + (i % b_len)];
		};

		std::uint32_t key_mixed1A = 0;
		std::uint32_t key_mixed2B = 0;

		const std::uint32_t b_pick = B_at(round_num);
		const std::uint32_t a_pick = A_at(round_num);

		// forward over A, keyed by one B word
		for (std::size_t i = 0; i < a_len; ++i)
		{
			key_mixed1A ^= ARX_forward(A_at(i), b_pick);
		}

		// backward over B, keyed by one A word
		for (std::size_t i = 0; i < b_len; ++i)
		{
			key_mixed2B ^= ARX_backward(B_at(i), a_pick);
		}

		const std::uint32_t rc = ROUND_CONSTANTS[round_num % ROUND_CONSTANTS.size()];
		const std::uint32_t mixed_key = (key_mixed1A - b_pick) ^ rc;
		const std::uint32_t final_key = mixed_key + (key_mixed2B ^ master_key[round_num % n]);

		return final_key;
	}

	void generate_round_keys(const std::vector<std::uint32_t>& master_key, std::size_t round_key_size)
	{
		round_keys.clear();
		round_keys.resize(round_key_size);

		for (std::size_t i = 0; i < round_key_size; ++i)
		{
			round_keys[i] = complex_key_mixing(master_key, i);
		}
	}

	// 工程修复点：预分配 + 扁平化 + 
	// 保持原“用 round_keys 作为 sbox_round_key”的思路
	void generate_sbox(const std::vector<std::uint32_t>& sbox_round_key)
	{
		if (sbox_round_key.size() < 2 || state_size == 0 || sbox_count == 0)
		{
			sbox_flat.clear();
			return;
		}

		sbox_flat.assign(sbox_count * state_size, 0u);

		// seed = ~(k0 ^ (k1 >> 16))
		std::mt19937 prng(~(sbox_round_key[0] ^ (sbox_round_key[1] >> 16)));

		// complex_key_mixing(sbox_round_key, i*state_size + j ^ prng())
		// (i*state_size + j) ^ prng()
		for (std::size_t i = 0; i < sbox_count; ++i)
		{
			const std::size_t row_base = i * state_size;
			for (std::size_t j = 0; j < state_size; ++j)
			{
				const std::size_t idx = (row_base + j) ^ static_cast<std::size_t>(prng());
				sbox_flat[row_base + j] = complex_key_mixing(sbox_round_key, idx);
			}
		}
	}

	inline const std::uint32_t* sbox_row_ptr(std::size_t sbox_index) const noexcept
	{
		return &sbox_flat[(sbox_index % sbox_count) * state_size];
	}

	// shift_rows / inverse_shift_rows
	void shift_rows(std::vector<std::uint32_t>& blocks)
	{
		if (blocks.size() != state_size)
			return;
		if (state_size % 4 != 0)
			return;

		const std::size_t rows = 4;
		const std::size_t cols = state_size / rows;

		// 逐行循环：row r 左旋 r
		for (std::size_t r = 1; r < rows; ++r)
		{
			const std::size_t base = r * cols;
			const std::size_t shift = r % cols;		  // cols 可能大，安全起见取模
			std::rotate(blocks.begin() + base,
						blocks.begin() + base + shift,
						blocks.begin() + base + cols);
		}
	}

	void inverse_shift_rows(std::vector<std::uint32_t>& blocks)
	{
		if (blocks.size() != state_size)
			return;
		if (state_size % 4 != 0)
			return;

		const std::size_t rows = 4;
		const std::size_t cols = state_size / rows;

		// 逐行逆旋转：row r 右旋 r
		for (std::size_t r = 1; r < rows; ++r)
		{
			const std::size_t base = r * cols;
			const std::size_t shift = r % cols;
			std::rotate(blocks.begin() + base,
						blocks.begin() + base + (cols - shift),
						blocks.begin() + base + cols);
		}
	}

	// 工程修复点：in-place round function，避免每轮分配 vector
	void round_function_forward_inplace(std::vector<std::uint32_t>& blocks, std::uint32_t round_key) const
	{
		const std::size_t sbox_index = (sbox_count == 0) ? 0 : (static_cast<std::size_t>(round_key) % sbox_count);
		const std::uint32_t* current_sbox = sbox_row_ptr(sbox_index);

		const std::uint32_t rk_lo = (round_key & 0x0000FFFFu);
		const std::uint32_t rk_hi = (round_key & 0xFFFF0000u);
		const std::uint32_t rot_rk7 = rotl32(round_key, 7u);
		const std::uint32_t rot_amt = round_key; // rotl32 会 &31

		for (std::size_t index = 0; index < state_size; ++index)
		{
			std::uint32_t block = blocks[index];

			// index % state_size == 0 
			// 只会在 index==0 触发
			if (index == 0)
			{
				block += current_sbox[index];
			}
			else
			{
				block ^= current_sbox[index];
			}

			const std::uint32_t rc = ROUND_CONSTANTS[index % ROUND_CONSTANTS.size()];
			block ^= ((~round_key & rc) ^ rk_lo);

			block = rotl32(block, rot_amt);

			block ^= rk_hi ^ rotl32(rc, 3u) ^ rot_rk7;

			blocks[index] = block;
		}
	}

	void round_function_backward_inplace(std::vector<std::uint32_t>& blocks, std::uint32_t round_key) const
	{
		const std::size_t sbox_index = (sbox_count == 0) ? 0 : (static_cast<std::size_t>(round_key) % sbox_count);
		const std::uint32_t* current_sbox = sbox_row_ptr(sbox_index);

		const std::uint32_t rk_lo = (round_key & 0x0000FFFFu);
		const std::uint32_t rk_hi = (round_key & 0xFFFF0000u);
		const std::uint32_t rot_rk7 = rotl32(round_key, 7u);
		const std::uint32_t rot_amt = round_key;

		for (std::size_t index = state_size; index-- > 0; )
		{
			std::uint32_t block = blocks[index];

			const std::uint32_t rc = ROUND_CONSTANTS[index % ROUND_CONSTANTS.size()];
			block ^= rk_hi ^ rotl32(rc, 3u) ^ rot_rk7;

			block = rotr32(block, rot_amt);

			block ^= ((~round_key & rc) ^ rk_lo);

			if (index == 0)
			{
				block -= current_sbox[index];
			}
			else
			{
				block ^= current_sbox[index];
			}

			blocks[index] = block;
		}
	}

	inline void block_exchange(std::vector<std::uint32_t>& blocks, std::size_t round_num) const noexcept
	{
		if (round_num < blocks.size())
		{
			std::swap(blocks[0], blocks[round_num]);
		}
	}

	inline void inverse_block_exchange(std::vector<std::uint32_t>& blocks, std::size_t round_num) const noexcept
	{
		if (round_num < blocks.size())
		{
			std::swap(blocks[0], blocks[round_num]);
		}
	}

	void ARX_encrypt_inplace(std::vector<std::uint32_t>& blocks) const
	{
		for (std::size_t r = 0; r < round_keys.size(); ++r)
		{
			round_function_forward_inplace(blocks, round_keys[r]);
			block_exchange(blocks, r);
		}
	}

	void ARX_decrypt_inplace(std::vector<std::uint32_t>& blocks) const
	{
		for (std::size_t r = round_keys.size(); r-- > 0; )
		{
			inverse_block_exchange(blocks, r);
			round_function_backward_inplace(blocks, round_keys[r]);
		}
	}

public:
	void regenerate_all(const std::vector<std::uint32_t>& master_key, std::size_t new_state_size, std::size_t new_sbox_count)
	{
		if (master_key.empty())
		{
			return;
		}

		if (new_state_size % 16 == 0 && new_state_size != 0)
		{
			state_size = new_state_size;
		}

		sbox_count = (new_sbox_count == 0) ? 1 : new_sbox_count;

		// round_keys 个数 = state_size
		generate_round_keys(master_key, state_size);
		generate_sbox(round_keys);
	}

	void resize_state(std::size_t new_state_size)
	{
		if (round_keys.empty())
		{
			return;
		}

		if (new_state_size % 16 != 0 || new_state_size == 0)
		{
			throw std::invalid_argument("State size must be a multiple of 16.");
		}

		state_size = new_state_size;
		generate_sbox(round_keys);
	}

	void resize_sbox_count(std::size_t new_sbox_count)
	{
		if (round_keys.empty())
		{
			return;
		}

		sbox_count = (new_sbox_count == 0) ? 1 : new_sbox_count;
		generate_sbox(round_keys);
	}

	std::vector<std::uint32_t> encrypt(const std::vector<std::uint32_t>& plaintext_blocks)
	{
		if (plaintext_blocks.size() % state_size != 0)
		{
			return plaintext_blocks;
		}

		std::vector<std::uint32_t> blocks = plaintext_blocks;
		shift_rows(blocks);
		ARX_encrypt_inplace(blocks);
		inverse_shift_rows(blocks);
		return blocks;
	}

	std::vector<std::uint32_t> decrypt(const std::vector<std::uint32_t>& ciphertext_blocks)
	{
		if (ciphertext_blocks.size() % state_size != 0)
		{
			return ciphertext_blocks;
		}

		std::vector<std::uint32_t> blocks = ciphertext_blocks;
		shift_rows(blocks);
		ARX_decrypt_inplace(blocks);
		inverse_shift_rows(blocks);
		return blocks;
	}

	// 工程修复点：避免每个 state 都构造临时 vector 再拷贝回去
	// 仍保持“多状态批处理”的外部接口语义
	std::vector<std::uint32_t> encrypt_multiple(const std::vector<std::uint32_t>& plaintext_blocks)
	{
		if (plaintext_blocks.empty())
		{
			return {};
		}
		if (plaintext_blocks.size() % state_size != 0)
		{
			return plaintext_blocks;
		}

		std::vector<std::uint32_t> out = plaintext_blocks;
		const std::size_t num_states = out.size() / state_size;

		// 复用一个 buffer，减少分配
		std::vector<std::uint32_t> state_buf;
		state_buf.resize(state_size);

		for (std::size_t i = 0; i < num_states; ++i)
		{
			const std::size_t base = i * state_size;
			std::copy(out.begin() + base, out.begin() + base + state_size, state_buf.begin());

			shift_rows(state_buf);
			ARX_encrypt_inplace(state_buf);
			inverse_shift_rows(state_buf);

			std::copy(state_buf.begin(), state_buf.end(), out.begin() + base);
		}

		return out;
	}

	std::vector<std::uint32_t> decrypt_multiple(const std::vector<std::uint32_t>& ciphertext_blocks)
	{
		if (ciphertext_blocks.empty())
		{
			return {};
		}
		if (ciphertext_blocks.size() % state_size != 0)
		{
			return ciphertext_blocks;
		}

		std::vector<std::uint32_t> out = ciphertext_blocks;
		const std::size_t num_states = out.size() / state_size;

		std::vector<std::uint32_t> state_buf;
		state_buf.resize(state_size);

		for (std::size_t i = 0; i < num_states; ++i)
		{
			const std::size_t base = i * state_size;
			std::copy(out.begin() + base, out.begin() + base + state_size, state_buf.begin());

			shift_rows(state_buf);
			ARX_decrypt_inplace(state_buf);
			inverse_shift_rows(state_buf);

			std::copy(state_buf.begin(), state_buf.end(), out.begin() + base);
		}

		return out;
	}

	explicit EtherealChaos(const std::vector<std::uint32_t>& master_key)
	{
		regenerate_all(master_key, 16, 64);
	}
};

#endif