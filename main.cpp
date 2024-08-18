#include <iostream>
#include <iomanip>
#include <chrono>

#include "EtherealChaos.hpp"
#include "AES.hpp"

// 基准测试函数，用于测量加密速度
inline void benchmark_encrypt(EtherealChaos& ec, const std::vector<uint32_t>& plaintext_blocks, size_t iterations)
{
	auto start = std::chrono::high_resolution_clock::now();

	for (size_t i = 0; i < iterations; ++i)
	{
		std::vector<uint32_t> ciphertext_blocks = ec.encrypt_multiple(plaintext_blocks);
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;

	std::cout << "Encryption took: " << duration.count() << " seconds for " << iterations << " iterations." << "(Single multi-state encryption setting iterations)" << std::endl;
}

// 基准测试函数，用于测量解密速度
inline void benchmark_decrypt(EtherealChaos& ec, const std::vector<uint32_t>& ciphertext_blocks, size_t iterations)
{
	auto start = std::chrono::high_resolution_clock::now();

	for (size_t i = 0; i < iterations; ++i)
	{
		std::vector<uint32_t> decrypted_blocks = ec.decrypt_multiple(ciphertext_blocks);
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;

	std::cout << "Decryption took: " << duration.count() << " seconds for " << iterations << " iterations." << "(Single multi-state decryption setting iterations)" << std::endl;
}

// 测试膨胀数据的加密时间
inline void test_scaling_encrypt(EtherealChaos& ec, size_t max_iterations)
{
	std::vector<uint32_t> plaintext_blocks(32, 0x12345678);  // 初始32个32位块的数据

	for (size_t i = 1; i <= max_iterations; ++i)
	{
		// 数据量翻倍
		plaintext_blocks.resize(plaintext_blocks.size() * 2, 0x12345678);

		auto start = std::chrono::high_resolution_clock::now();

		// 进行一次加密
		std::vector<uint32_t> ciphertext_blocks = ec.encrypt_multiple(plaintext_blocks);

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration = end - start;

		std::cout << "Encryption took: " << duration.count() << " seconds for data size: " << plaintext_blocks.size() << " 32-bit blocks (" << (plaintext_blocks.size() * 32) << " bits)." << std::endl;
	}
}

// 测试膨胀数据的解密时间
inline void test_scaling_decrypt(EtherealChaos& ec, size_t max_iterations)
{
	std::vector<uint32_t> plaintext_blocks(32, 0x12345678);							// 初始32个32位块的数据
	std::vector<uint32_t> ciphertext_blocks = ec.encrypt_multiple(plaintext_blocks);	// 初始加密

	for (size_t i = 1; i <= max_iterations; ++i)
	{
		// 数据量翻倍
		plaintext_blocks.resize(plaintext_blocks.size() * 2, 0x12345678);
		ciphertext_blocks = ec.encrypt_multiple(plaintext_blocks);  // 重新加密对应的新数据量

		auto start = std::chrono::high_resolution_clock::now();

		// 进行一次解密
		std::vector<uint32_t> decrypted_blocks = ec.decrypt_multiple(ciphertext_blocks);

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration = end - start;

		std::cout << "Decryption took: " << duration.count() << " seconds for data size: " << ciphertext_blocks.size() << " 32-bit blocks (" << (ciphertext_blocks.size() * 32) << " bits)." << std::endl;
	}
}

/********************/

inline void benchmark_encrypt(AES& aes, const std::vector<uint32_t>& plaintext_blocks, size_t iterations) {
	std::chrono::duration<double> total_duration(0);

	for (size_t i = 0; i < iterations; ++i) {
		aes.encrypt_uint32(plaintext_blocks);

		// 累计每次加密的时间
		total_duration += aes.end_time - aes.start_time;
	}

	std::cout << "Iteration " << iterations << ": Encryption took " << total_duration.count() << " seconds." << std::endl;
}

inline void benchmark_decrypt(AES& aes, const std::vector<uint32_t>& ciphertext_blocks, size_t iterations) {
	std::chrono::duration<double> total_duration(0);

	for (size_t i = 0; i < iterations; ++i) {
		aes.decrypt_uint32(ciphertext_blocks);

		// 累计每次解密的时间
		total_duration += aes.end_time - aes.start_time;
	}

	std::cout << "Iteration " << iterations << ": Decryption took " << total_duration.count() << " seconds." << std::endl;
}

inline void test_scaling_encrypt(AES& aes, size_t max_iterations) {
	std::vector<uint32_t> plaintext_blocks(4, 0x12345678); // AES 的初始块数为 4 个 32-bit 块（128-bit）

	for (size_t i = 1; i <= max_iterations; ++i) {
		// 使用 AES 类的成员变量记录时间
		std::chrono::duration<double> duration(0);

		// 加密整个数据块
		aes.encrypt_uint32(plaintext_blocks);
		duration += aes.end_time - aes.start_time;

		// 打印结果
		std::cout << "Encryption took: " << duration.count() << " seconds for data size: "
			<< plaintext_blocks.size() << " 32-bit blocks (" << (plaintext_blocks.size() * 32) << " bits)." << std::endl;

		// 数据量翻倍，用于下一次迭代
		plaintext_blocks.resize(plaintext_blocks.size() * 2, 0x12345678);
	}
}

inline void test_scaling_decrypt(AES& aes, size_t max_iterations) {
	std::vector<uint32_t> plaintext_blocks(4, 0x12345678);  // 初始的 4 个 32-bit 块
	std::vector<uint32_t> ciphertext_blocks = aes.encrypt_uint32(plaintext_blocks); // 初始加密

	for (size_t i = 1; i <= max_iterations; ++i) {
		// 使用 AES 类的成员变量记录时间
		std::chrono::duration<double> duration(0);

		// 解密整个数据块
		aes.decrypt_uint32(ciphertext_blocks);
		duration += aes.end_time - aes.start_time;

		// 打印结果
		std::cout << "Decryption took: " << duration.count() << " seconds for data size: "
			<< ciphertext_blocks.size() << " 32-bit blocks (" << (ciphertext_blocks.size() * 32) << " bits)." << std::endl;

		// 数据量翻倍，用于下一次迭代
		plaintext_blocks.resize(plaintext_blocks.size() * 2, 0x12345678);
		ciphertext_blocks = aes.encrypt_uint32(plaintext_blocks); // 重新加密新的数据量
	}
}

inline void BenchmarkEtherealChaos()
{
	// 示例使用
	std::vector<uint32_t> master_keys =
	{
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
	};
	EtherealChaos ec(master_keys);

	// 示例输入：32个32位元素
	std::vector<uint32_t> plaintext_blocks = {
		0x11111111, 0x22222222, 0x33333333, 0x44444444,
		0x55555555, 0x66666666, 0x77777777, 0x88888888,
		0x99999999, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC,
		0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF, 0x00000000,
		0x11111111, 0x22222222, 0x33333333, 0x44444444,
		0x55555555, 0x66666666, 0x77777777, 0x88888888,
		0x99999999, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC,
		0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF, 0x00000000
	};

	// 进行一次加密，获取密文块
	std::vector<uint32_t> ciphertext_blocks = ec.encrypt_multiple(plaintext_blocks);

	// 单次多块加密/解密设置迭代次数
	size_t iterations = 10240000;

	// 进行加密基准测试
	std::cout << "Starting Encryption Benchmark:" << std::endl;
	benchmark_encrypt(ec, plaintext_blocks, iterations);

	// 进行解密基准测试
	std::cout << "Starting Decryption Benchmark:" << std::endl;
	benchmark_decrypt(ec, ciphertext_blocks, iterations);

	// 进行膨胀数据测试，指定最大迭代次数
	size_t max_iterations = 24;

	EtherealChaos ec2(master_keys);

	std::cout << "Starting Encryption Scaling Test:" << std::endl;
	test_scaling_encrypt(ec2, max_iterations);

	std::cout << "\nStarting Decryption Scaling Test:" << std::endl;
	test_scaling_decrypt(ec2, max_iterations);
}

inline void BenchmarkAES256()
{
	// 示例使用
	std::vector<uint32_t> master_key = {
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321
	};

	AES aes(master_key);

	// 示例输入：4个32位元素
	std::vector<uint32_t> plaintext_blocks = {
		0x11111111, 0x22222222, 0x33333333, 0x44444444
	};

	// 进行一次加密，获取密文块
	std::vector<uint32_t> ciphertext_blocks = aes.encrypt_uint32(plaintext_blocks);

	// 调整后的迭代次数，确保处理的数据量与 Ethereal Chaos 相同
	size_t iterations = 10240000;

	// 进行加密基准测试
	std::cout << "Starting Encryption Benchmark:" << std::endl;
	benchmark_encrypt(aes, plaintext_blocks, iterations);

	// 进行解密基准测试
	std::cout << "Starting Decryption Benchmark:" << std::endl;
	benchmark_decrypt(aes, ciphertext_blocks, iterations);

	// 调整后的膨胀测试，确保处理的数据量与 Ethereal Chaos 相同
	size_t max_iterations = 24 + 4;

	std::cout << "Starting Encryption Scaling Test:" << std::endl;
	test_scaling_encrypt(aes, max_iterations);

	std::cout << "\nStarting Decryption Scaling Test:" << std::endl;
	test_scaling_decrypt(aes, max_iterations);
}

inline void TestVectorEtherealChaos()
{
	// 示例使用
	std::vector<uint32_t> master_keys =
	{
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
		0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321,
	};
	EtherealChaos ec(master_keys);

	// 示例输入：32个32位元素
	std::vector<uint32_t> plaintext_blocks = {
		0x11111111, 0x22222222, 0x33333333, 0x44444444,
		0x55555555, 0x66666666, 0x77777777, 0x88888888,
		0x99999999, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC,
		0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF, 0x00000000,
		0x11111111, 0x22222222, 0x33333333, 0x44444444,
		0x55555555, 0x66666666, 0x77777777, 0x88888888,
		0x99999999, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC,
		0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF, 0x00000000
	};

	// 进行一次加密，获取密文块
	std::vector<uint32_t> ciphertext_blocks = ec.encrypt_multiple(plaintext_blocks);
	// 进行一次解密，获取计算的明文块
	std::vector<uint32_t> recomputed_plaintext = ec.decrypt_multiple(ciphertext_blocks);

	// 输出 TestVector 数据
	std::cout << "TestVector:\n";

	std::cout << "Master Keys: \n";
	for (const auto& key : master_keys) {
		std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << key << " ";
	}
	std::cout << "\n\n";

	std::cout << "Plaintext: \n";
	for (const auto& block : plaintext_blocks) {
		std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << block << " ";
	}
	std::cout << "\n\n";

	std::cout << "Ciphertext: \n";
	for (const auto& block : ciphertext_blocks) {
		std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << block << " ";
	}
	std::cout << "\n\n";

	std::cout << "Recomputed data: \n";
	for (const auto& block : recomputed_plaintext) {
		std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << block << " ";
	}
	std::cout << "\n";
}


int main()
{
	//BenchmarkEtherealChaos();
	//BenchmarkAES256();
	TestVectorEtherealChaos();
}