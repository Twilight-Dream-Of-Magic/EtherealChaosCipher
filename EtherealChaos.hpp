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
	std::vector<std::vector<uint32_t>> SBOX_2D;
	std::vector<uint32_t>			   round_keys;
	size_t							   state_size = 16;	 // Current state size (must be a multiple of 16) (Columns of SBOX_2D)
	size_t							   sbox_count;		 // Rows of SBOX_2D

	uint32_t rotate_left( uint32_t value, uint32_t shift )
	{
		return ( value << shift ) | ( value >> ( 32 - shift ) );
	}

	uint32_t rotate_right( uint32_t value, uint32_t shift )
	{
		return ( value >> shift ) | ( value << ( 32 - shift ) );
	}

	uint32_t ARX_forward( uint32_t block, uint32_t key_part )
	{
		block = ( block + key_part );
		block = rotate_left( block, key_part % 32 );
		block ^= key_part;
		return block;
	}

	uint32_t ARX_backward( uint32_t block, uint32_t key_part )
	{
		block ^= key_part;
		block = rotate_right( block, key_part % 32 );
		block = ( block - key_part );
		return block;
	}

	uint32_t complex_key_mixing( const std::vector<uint32_t>& master_key, size_t round_num )
	{
		size_t key_half_size = master_key.size() / 2;

		// Split the master key array into two parts
		std::vector<uint32_t> key_part1A( master_key.begin(), master_key.begin() + key_half_size );
		std::vector<uint32_t> key_part1B( master_key.begin() + key_half_size, master_key.end() );

		std::vector<uint32_t> key_part2A = key_part1A;
		std::vector<uint32_t> key_part2B = key_part1B;

		// Performs forward and reverse ARX operations
		uint32_t key_mixed1A = 0;
		uint32_t key_mixed2B = 0;

		for ( size_t i = 0; i < key_part1A.size(); ++i )
		{
			key_mixed1A ^= ARX_forward( key_part1A[ i ], key_part1B[ round_num % key_part1B.size() ] );
		}
		for ( size_t i = 0; i < key_part2B.size(); ++i )
		{
			key_mixed2B ^= ARX_backward( key_part2B[ i ], key_part2A[ round_num % key_part1A.size() ] );
		}

		// Mix and generate the final key
		uint32_t mixed_key = ( key_mixed1A - key_part1B[ round_num % key_part1B.size() ] ) ^ ROUND_CONSTANTS[ round_num % ROUND_CONSTANTS.size() ];
		uint32_t final_key = ( mixed_key + ( key_mixed2B ^ master_key[ round_num % master_key.size() ] ) );

		return final_key;
	}

	void generate_round_keys( const std::vector<uint32_t>& master_key, size_t round_key_size )
	{
		this->round_keys.clear();
		for ( size_t i = 0; i < round_key_size; ++i )
		{
			this->round_keys.push_back( complex_key_mixing( master_key, i ) );
		}
	}

	void generate_sbox( const std::vector<uint32_t>& sbox_round_key )
	{
		if ( sbox_round_key.empty() )
		{
			return;
		}

		SBOX_2D.clear();
		std::mt19937 prng( ~( sbox_round_key[ 0 ] ^ ( sbox_round_key[ 1 ] >> 16 ) ) );

		// The size of each SBOX_2D subset (subset_size) is equal to state_size

		for ( size_t i = 0; i < this->sbox_count; ++i )
		{
			std::vector<uint32_t> sbox_subset;
			for ( size_t j = 0; j < this->state_size; ++j )
			{
				sbox_subset.push_back( complex_key_mixing( sbox_round_key, i * this->state_size + j ^ prng() ) & 0xFFFFFFFF );
			}
			SBOX_2D.push_back( sbox_subset );
		}
	}

	void shift_rows( std::vector<uint32_t>& blocks )
	{
		size_t				  n = blocks.size();
		std::vector<uint32_t> temp = blocks;
		// 4x4 Matrix, rotating rows
		for ( size_t i = 1; i < 4; ++i )
		{
			std::rotate( temp.begin() + i * 4, temp.begin() + i * 4 + i, temp.begin() + ( i + 1 ) * 4 );
		}
		blocks = temp;
	}

	void inverse_shift_rows( std::vector<uint32_t>& blocks )
	{
		size_t				  n = blocks.size();
		std::vector<uint32_t> temp = blocks;
		// 4x4 Matrix, inverse rotating rows
		for ( size_t i = 1; i < 4; ++i )
		{
			std::rotate( temp.begin() + i * 4, temp.begin() + ( i + 1 ) * 4 - i, temp.begin() + ( i + 1 ) * 4 );
		}
		blocks = temp;
	}

	std::vector<uint32_t> round_function_forward( const std::vector<uint32_t>& blocks, uint32_t round_key )
	{
		std::vector<uint32_t> new_blocks( blocks.size() );

		// Select the same subset of SBOX_2D based on the round key
		size_t						 sbox_index = round_key % SBOX_2D.size();
		const std::vector<uint32_t>& current_sbox = SBOX_2D[ sbox_index ];

		for ( size_t index = 0; index < state_size; ++index )
		{
			uint32_t block = blocks[ index ];
			if ( index % state_size == 0 )
			{
				block += current_sbox[ index % state_size ];
			}
			else
			{
				block ^= current_sbox[ index % state_size ];
			}
			block ^= ( ( ~round_key & ROUND_CONSTANTS[ index % ROUND_CONSTANTS.size() ] ) ^ ( round_key & 0x0000FFFF ) );
			block = rotate_left( block, round_key % 32 );
			block ^= ( round_key & 0xFFFF0000 ) ^ rotate_left( ROUND_CONSTANTS[ index % ROUND_CONSTANTS.size() ], 3 ) ^ rotate_left( round_key, 7 );
			new_blocks[ index ] = block;
		}
		return new_blocks;
	}

	std::vector<uint32_t> round_function_backward( const std::vector<uint32_t>& blocks, uint32_t round_key )
	{
		std::vector<uint32_t> new_blocks( blocks.size() );

		// Select the same subset of SBOX_2D based on the round key
		size_t						 sbox_index = round_key % SBOX_2D.size();
		const std::vector<uint32_t>& current_sbox = SBOX_2D[ sbox_index ];

		for ( size_t index = state_size; index-- > 0; )
		{
			uint32_t block = blocks[ index ];
			block ^= ( round_key & 0xFFFF0000 ) ^ rotate_left( ROUND_CONSTANTS[ index % ROUND_CONSTANTS.size() ], 3 ) ^ rotate_left( round_key, 7 );
			block = rotate_right( block, round_key % 32 );
			block ^= ( ( ~round_key & ROUND_CONSTANTS[ index % ROUND_CONSTANTS.size() ] ) ^ ( round_key & 0x0000FFFF ) );
			if ( index % state_size == 0 )
			{
				block -= current_sbox[ index % state_size ];
			}
			else
			{
				block ^= current_sbox[ index % state_size ];
			}
			new_blocks[ index ] = block;
		}
		return new_blocks;
	}

	void block_exchange( std::vector<uint32_t>& blocks, size_t round_num )
	{
		if ( round_num < blocks.size() )
		{
			std::swap( blocks[ 0 ], blocks[ round_num ] );
		}
	}

	void inverse_block_exchange( std::vector<uint32_t>& blocks, size_t round_num )
	{
		if ( round_num < blocks.size() )
		{
			std::swap( blocks[ 0 ], blocks[ round_num ] );
		}
	}

	std::vector<uint32_t> ARX_encrypt( std::vector<uint32_t> blocks )
	{
		for ( size_t r = 0; r < round_keys.size(); ++r )
		{
			blocks = round_function_forward( blocks, round_keys[ r ] );
			block_exchange( blocks, r );
		}
		return blocks;
	}

	std::vector<uint32_t> ARX_decrypt( std::vector<uint32_t> blocks )
	{
		for ( size_t r = round_keys.size(); r-- > 0; )
		{
			inverse_block_exchange( blocks, r );
			blocks = round_function_backward( blocks, round_keys[ r ] );
		}
		return blocks;
	}

public:

	void regenerate_all( const std::vector<uint32_t>& master_key, size_t new_state_size, size_t new_sbox_count )
	{
		if(master_key.empty())
		{
			return;
		}

		if ( new_state_size % 16 == 0 )
		{
			this->state_size = new_state_size;
		}

		this->sbox_count = new_sbox_count;
		generate_round_keys( master_key, this->state_size ); // Master key array Generate per-round subkey array
		generate_sbox( this->round_keys ); // Master key array Regenerate SBOX_2D array
	}

	void resize_state( size_t new_state_size )
	{
		if(round_keys.empty())
		{
			return;
		}

		if ( new_state_size % 16 != 0 )
		{
			throw std::invalid_argument( "State size must be a multiple of 16." );
		}

		this->state_size = new_state_size;
		generate_sbox( this->round_keys ); // Update SBOX_2D to accommodate new state size
	}

	void resize_sbox_count( size_t new_sbox_count )
	{
		if(round_keys.empty())
		{
			return;
		}

		this->sbox_count = new_sbox_count;
		generate_sbox( this->round_keys );  // Per-round subkey array Regenerate SBOX_2D array
	}

	std::vector<uint32_t> encrypt( const std::vector<uint32_t>& plaintext_blocks )
	{
		if ( plaintext_blocks.size() % state_size != 0 )
		{
			return plaintext_blocks;
		}

		std::vector<uint32_t> blocks = plaintext_blocks;
		shift_rows( blocks );  // initial permutation
		blocks = ARX_encrypt( blocks );
		inverse_shift_rows( blocks );  // final permutation
		return blocks;
	}

	std::vector<uint32_t> decrypt( const std::vector<uint32_t>& ciphertext_blocks )
	{
		if ( ciphertext_blocks.size() % state_size != 0 )
		{
			return ciphertext_blocks;
		}

		std::vector<uint32_t> blocks = ciphertext_blocks;
		shift_rows( blocks );  // initial permutation
		blocks = ARX_decrypt( blocks );
		inverse_shift_rows( blocks );  // final permutation
		return blocks;
	}

	std::vector<uint32_t> encrypt_multiple( const std::vector<uint32_t>& plaintext_blocks )
	{
		std::vector<uint32_t> encrypted_blocks(plaintext_blocks.size()); // 预分配空间
		size_t				  num_states = plaintext_blocks.size() / this->state_size;

		for ( size_t i = 0; i < num_states; ++i )
		{
			// 提取当前的state
			std::vector<uint32_t> current_state( plaintext_blocks.begin() + i * this->state_size, plaintext_blocks.begin() + ( i + 1 ) * this->state_size );

			// 调用原有的加密函数
			std::vector<uint32_t> encrypted_state = encrypt( current_state );

			// 将加密后的state直接赋值到结果中
			for (size_t j = 0; j < state_size; ++j) {
				encrypted_blocks[i * state_size + j] = encrypted_state[j];
			}
		}

		return encrypted_blocks;
	}

	std::vector<uint32_t> decrypt_multiple( const std::vector<uint32_t>& ciphertext_blocks )
	{
		std::vector<uint32_t> decrypted_blocks(ciphertext_blocks.size()); // 预分配空间
		size_t				  num_states = ciphertext_blocks.size() / this->state_size;

		for ( size_t i = 0; i < num_states; ++i )
		{
			// 提取当前的state
			std::vector<uint32_t> current_state( ciphertext_blocks.begin() + i * this->state_size, ciphertext_blocks.begin() + ( i + 1 ) * this->state_size );

			// 调用原有的解密函数
			std::vector<uint32_t> decrypted_state = decrypt( current_state );

			// 将解密后的state直接赋值到结果中
			for (size_t j = 0; j < state_size; ++j) {
				decrypted_blocks[i * state_size + j] = decrypted_state[j];
			}
		}

		return decrypted_blocks;
	}

	EtherealChaos( const std::vector<uint32_t>& master_key )
	{
		regenerate_all( master_key, 16, 64 ); // The default number of SBOX_2D rows is 64 (16 * 64 = 1024), and this function will also regenerate the key for each round.
	}
};

#endif