#ifndef EZGZ_HPP
#define EZGZ_HPP

#include <array>
#include <cstring>
#include <span>
#include <charconv>
#include <vector>
#include <numeric>
#include <optional>
#include <functional>
#include <variant>
#include <memory>
#include <istream>
#ifndef EZGZ_NO_FILE
#include <fstream>
#endif
#include <cassert>
#include <bit>
#include <iostream>

#ifdef _MSC_VER
#pragma warning(disable : 4244)
#endif

namespace EzGz {

template <typename T>
concept DecompressionSettings = std::constructible_from<typename T::Checksum> && requires(typename T::Checksum checksum) {
	int(T::maxOutputBufferSize);
	int(T::minOutputBufferSize);
	int(T::inputBufferSize);
	int(checksum());
	int(checksum(std::span<const uint8_t>()));
	bool(T::verifyChecksum);
};

template <typename T>
concept BasicStringType = std::constructible_from<T> && requires(T value) {
	value += 'a';
	std::string_view(value);
};

struct NoChecksum { // Noop
	int operator() () { return 0; }
	int operator() (std::span<const uint8_t>) { return 0; }
};

struct MinDecompressionSettings {
	constexpr static int maxOutputBufferSize = 32768 * 2 + 258;
	constexpr static int minOutputBufferSize = std::min(32768, maxOutputBufferSize / 2); // Max offset + max copy size
	constexpr static int inputBufferSize = 33000;
	using Checksum = NoChecksum;
	constexpr static bool verifyChecksum = false;
	using StringType = std::string;
};

namespace Detail {

constexpr std::array<uint32_t, 256> generateBasicCrc32LookupTable() {
	constexpr uint32_t reversedPolynomial = 0xedb88320;
	std::array<uint32_t, 256> result = {};
	for (int i = 0; i < std::ssize(result); i++) {
		result[i] = i;
		for (auto j = 0; j < 8; j++)
			result[i] = (result[i] >> 1) ^ ((result[i] & 0x1) * reversedPolynomial);
	}
	return result;
}

constexpr std::array<uint32_t, 256> basicCrc32LookupTable = generateBasicCrc32LookupTable();

static constexpr std::array<uint32_t, 256> generateNextCrc32LookupTableSlice(const std::array<uint32_t, 256>& previous) {
	std::array<uint32_t, 256> result = {};
	for (int i = 0; i < std::ssize(result); i++) {
		result[i] = (previous[i] >> 8) ^ (basicCrc32LookupTable[previous[i] & 0xff]);
	}
	return result;
}

template <int Slice>
struct CrcLookupTable {
	constexpr static std::array<uint32_t, 256> data = generateNextCrc32LookupTableSlice(CrcLookupTable<Slice - 1>::data);
};

template <>
struct CrcLookupTable<0> {
	constexpr static const std::array<uint32_t, 256> data = basicCrc32LookupTable;
};
}

class LightCrc32 {
	uint32_t state = 0xffffffffu;

public:
	uint32_t operator() () { return ~state; }
	uint32_t operator() (std::span<const uint8_t> input) {
		for (auto it : input) {
			const uint8_t tableIndex = static_cast<uint8_t>(state ^ it);
			state = (state >> 8) ^ Detail::basicCrc32LookupTable[tableIndex];
		}
		return ~state; // Invert all bits at the end
	}
};

// Inspired by https://create.stephan-brumme.com/crc32/
class FastCrc32 {
	uint32_t state = 0xffffffffu;

	constexpr static int chunkSize = 16;
	struct CodingChunk {
		std::array<uint8_t, 16> chunk;

		constexpr static std::array<const std::array<uint32_t, 256>, chunkSize> lookupTables = {
			Detail::CrcLookupTable<0>::data, Detail::CrcLookupTable<1>::data, Detail::CrcLookupTable<2>::data, Detail::CrcLookupTable<3>::data,
			Detail::CrcLookupTable<4>::data, Detail::CrcLookupTable<5>::data, Detail::CrcLookupTable<6>::data, Detail::CrcLookupTable<7>::data,
			Detail::CrcLookupTable<8>::data, Detail::CrcLookupTable<9>::data, Detail::CrcLookupTable<10>::data, Detail::CrcLookupTable<11>::data,
			Detail::CrcLookupTable<12>::data, Detail::CrcLookupTable<13>::data, Detail::CrcLookupTable<14>::data, Detail::CrcLookupTable<15>::data};

		template <int Index = 0, int Size = chunkSize>
		uint32_t process() {
			if constexpr(Size == 1) {
				return lookupTables[chunkSize - 1 - Index][chunk[Index]];
			} else {
				return process<Index, Size / 2>() ^ process<Index + Size / 2, Size / 2>();
			}
		}
	};

public:
	uint32_t operator() () { return ~state; }
	uint32_t operator() (std::span<const uint8_t> input) {
		ptrdiff_t position = 0;
		for ( ; position + chunkSize < std::ssize(input); position += chunkSize) {
			CodingChunk chunk;
			memcpy(chunk.chunk.data() + sizeof(state), input.data() + position + sizeof(state), chunkSize - sizeof(state));
			union {
				std::array<uint8_t, sizeof(state)> bytes;
				uint32_t number = 0;
			} stateBytes;
			stateBytes.number = state;
			if constexpr (std::endian::native == std::endian::big) {
				for (int i = 0; i < std::ssize(stateBytes.bytes) / 2; i++) {
					std::swap(stateBytes.bytes[i], stateBytes.bytes[std::ssize(stateBytes.bytes) - 1 - i]);
				}
			}
			stateBytes.number ^= *reinterpret_cast<const uint32_t*>(input.data() + position);
			memcpy(chunk.chunk.data(), reinterpret_cast<const uint8_t*>(&stateBytes.number), sizeof(state));

			state = chunk.process();
		}

		for ( ; position < std::ssize(input); position++) {
			const uint8_t tableIndex = static_cast<uint8_t>(state ^ input[position]);
			state = (state >> 8) ^ Detail::basicCrc32LookupTable[tableIndex];
		}
		return ~state; // Invert all bits at the end
	}
};

struct DefaultDecompressionSettings : MinDecompressionSettings {
	constexpr static int maxOutputBufferSize = 100000;
	constexpr static int inputBufferSize = 100000;
	using Checksum = FastCrc32;
	constexpr static bool verifyChecksum = true;
};

namespace Detail {

template <typename Builder>
struct ArrayFiller {
	Builder builder;
	using BuildingType = decltype(builder(0));
	constexpr ArrayFiller(Builder&& builder) : builder(std::move(builder)) {}

	template <size_t Size>
	constexpr operator std::array<BuildingType, Size>() {
		return build(std::make_index_sequence<Size>());
	}
private:
	template <size_t... Indexes>
	constexpr std::array<BuildingType, sizeof...(Indexes)> build(std::index_sequence<Indexes...>) {
		return std::array<BuildingType, sizeof...(Indexes)>{builder(Indexes)...};
	}
};

static constexpr std::array<uint8_t, 19> codeCodingReorder = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

static constexpr std::array<uint8_t, 256> reversedBytes = ArrayFiller([] (int uninverted) constexpr {
	return uint8_t((uninverted * uint64_t(0x0202020202) & uint64_t(0x010884422010)) % 0x3ff); // Reverse a byte
});

// Provides access to input stream as chunks of contiguous data
template <DecompressionSettings Settings>
class ByteInput {
	std::array<uint8_t, Settings::inputBufferSize + sizeof(uint32_t)> buffer = {};
	std::function<int(std::span<uint8_t> batch)> readMore;
	int position = 0;
	int filled = 0;
	int refillSome() {
		if (position > std::ssize(buffer) / 2) {
			filled -= position;
			memmove(buffer.data(), buffer.data() + position, filled);
			position = 0;
		}
		int added = readMore(std::span<uint8_t>(buffer.begin() + filled, buffer.end()));
		filled += added;
		return added;
	}

	void ensureSize(int bytes) {
		while ((position) + bytes > filled) [[unlikely]] {
			int added = refillSome();
			if (added == 0) {
				throw std::runtime_error("Unexpected end of stream");
			}
		}
	}

public:
	ByteInput(std::function<int(std::span<uint8_t> batch)> readMoreFunction) : readMore(readMoreFunction) {}

	// Note: May not get as many bytes as necessary, would need to be called multiple times
	std::span<const uint8_t> getRange(int size) {
		if (position + size >= filled) {
			refillSome();
		}
		ptrdiff_t start = position;
		int available = std::min<int>(size, static_cast<int>(filled - start));
		position += available;
		return {buffer.begin() + start, buffer.begin() + start + available};
	}

	uint64_t getBytes(int amount) {
		return getInteger<int64_t>(amount);
	}

	template <typename IntType>
	uint64_t getInteger(int bytes = sizeof(IntType)) {
		IntType result = 0;
		ensureSize(bytes);
		memcpy(&result, &buffer[position], bytes);
		position += bytes;
		return result;
	}

	// Can return only up to the size of the last read
	void returnBytes(int amount) {
		position -= amount;
	}

	template <int MaxTableSize>
	auto encodedTable(int realSize, const std::array<uint8_t, 256>& codeCodingLookup, const std::array<uint8_t, codeCodingReorder.size()>& codeCodingLengths);
};

template <typename T>
concept ByteReader = requires(T reader) {
	reader.returnBytes(1);
	std::span<const uint8_t>(reader.getRange(6));
};

// Provides optimised access to data from a ByteInput by bits
template <ByteReader ByteInputType>
class BitReader {
	ByteInputType* input;
	int bitsLeft = 0;
	uint64_t data = 0; // Invariant - lowest bit is the first valid
	static constexpr int minimumBits = 16; // The specification doesn't require any reading by bits that are longer than 16 bits

	void refillIfNeeded() {
		if (bitsLeft < minimumBits) {
			std::span<const uint8_t> added = input->getRange(sizeof(data) - (minimumBits / 8));
			union {
				std::array<uint8_t, sizeof(uint64_t)> bytes;
				uint64_t number = 0;
			} dataAdded;
			if constexpr (std::endian::native == std::endian::little) {
				memcpy(dataAdded.bytes.data(), added.data(), std::ssize(added));
			} else if constexpr (std::endian::native == std::endian::big) {
				for (int i = 0; i < std::ssize(added); i++) {
					dataAdded.bytes[sizeof(data) - i] = added[i];
				}
			} else static_assert(std::endian::native == std::endian::big || std::endian::native == std::endian::little);
			dataAdded.number <<= bitsLeft;
			data += dataAdded.number;
			bitsLeft += (added.size() << 3);
		}
	}

	static constexpr std::array<uint16_t, 17> upperRemovals = {0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
			0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff};

public:

	BitReader(ByteInputType* byteInput) : input(byteInput) {}
	BitReader(BitReader&& other) noexcept : input(other.input), bitsLeft(other.bitsLeft), data(other.data) {
		other.input = nullptr;
	}
	BitReader(const BitReader&) = delete;
	BitReader& operator=(BitReader&& other) {
		if (input)
			input->returnBytes(bitsLeft >> 3);
		input = other.input;
		other.input = nullptr;
		bitsLeft = other.bitsLeft;
		data = other.data;
		return *this;
	}
	BitReader& operator=(const BitReader&) = delete;
	~BitReader() {
		if (input)
			input->returnBytes(bitsLeft >> 3);
	}

	// Up to 16 bits, unwanted bits blanked
	uint16_t getBits(int amount) {
		refillIfNeeded();
		if (bitsLeft < amount) [[unlikely]] {
			throw std::runtime_error("Run out of data");
		}
		uint16_t result = static_cast<uint16_t>(data);
		data >>= amount;
		bitsLeft -= amount;
		result &= upperRemovals[amount];
		return result;
	}

	// Provides 8 bits, the functor must return how many of them were actually wanted
	template <typename ReadAndTellHowMuchToConsume>
	void peekAByteAndConsumeSome(const ReadAndTellHowMuchToConsume& readAndTellHowMuchToConsume) {
		refillIfNeeded();
		uint8_t pulled = data;
		auto consumed = readAndTellHowMuchToConsume(pulled);
		if (bitsLeft < consumed) [[unlikely]] {
			throw std::runtime_error("Run out of data");
		}
		data >>= consumed;
		bitsLeft -= consumed;
	}

	// Uses the table in the specification to determine how many bytes are copied
	int parseLongerSize(int partOfSize) {
		if (partOfSize != 31) {
			// Sizes in this range take several extra bits
			int size = partOfSize;
			int nextBits = (size - 7) >> 2;
			auto additionalBits = getBits(nextBits);
			size++; // Will ease the next line
			size = ((((size & 0x3) << nextBits) | additionalBits)) + ((1 << (size >> 2)) + 3); // This is a generalisation of the size table at 3.2.5
			return size;
		} else {
			return 258; // If the value is maximmum, it means the size is 258
		}
	}

	// Uses the table in the specification to determine distance from where bytes are copied
	int parseLongerDistance(int partOfDistance) {
		int readMore = (partOfDistance - 3) >> 1;
		auto moreBits = getBits(readMore);
		constexpr static std::array<int, 30> distanceOffsets = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33,
				49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
		int distance = distanceOffsets[partOfDistance - 1] + moreBits;
		return distance;
	}
};

// Handles output of decompressed data, filling bytes from past bytes and chunking. Consume needs to be called to empty it
template <DecompressionSettings Settings>
class ByteOutput {
	std::array<char, Settings::maxOutputBufferSize> buffer = {};
	int used = 0; // Number of bytes filled in the buffer (valid data must start at index 0)
	int consumed = 0; // The last byte that was returned by consume()
	bool expectsMore = true; // If we expect more data to be present
	typename Settings::Checksum checksum = {};

	void checkSize(int added = 1) {
		if (used + added > std::ssize(buffer)) [[unlikely]] {
			throw std::logic_error("Writing more bytes than available, probably an internal bug");
		}
	}

public:
	int available() {
		return buffer.size() - used;
	}

	std::span<const char> consume(const int bytesToKeep = 0) {
		// Last batch has to be handled differently
		if (!expectsMore) [[unlikely]] {
			std::span<const char> returning = std::span<const char>(buffer.data() + consumed, used - consumed);
			checksum(std::span<uint8_t>(reinterpret_cast<uint8_t*>(buffer.data() + consumed), used - consumed));

			consumed = used;
			return returning;
		}

		// Clean the space from the previous consume() call
		int bytesKept = std::min(bytesToKeep, consumed);
		int removing = consumed - bytesKept;
		int minimum = Settings::minOutputBufferSize - used + consumed; // Ensure we keep enough bytes that the operation will end with less valid data in the buffer than the mandatory minimum
		if (bytesKept < minimum) {
			bytesKept = minimum;
			removing = consumed - bytesKept;
		}
		if (removing < 0) [[unlikely]] {
			throw std::logic_error("consume() cannot keep more bytes than it provided before");
		}
		memmove(buffer.data(), buffer.data() + removing, used - removing);
		used -= removing;
		consumed = used; // Make everything in the buffer available (except the data returned earlier)

		// Return a next batch
		checksum(std::span<uint8_t>(reinterpret_cast<uint8_t*>(buffer.data() + bytesKept), consumed - bytesKept));
		return std::span<const char>(buffer.data() + bytesKept, consumed - bytesKept);
	}

	void addByte(char byte) {
		checkSize();
		buffer[used] = byte;
		used++;
	}

	void addBytes(std::span<const char> bytes) {
		checkSize(bytes.size());
		memcpy(buffer.data() + used, bytes.data(), bytes.size());
		used += bytes.size();
	}

	void repeatSequence(int length, int distance) {
		checkSize(length);
		int written = 0;
		while (written < length) {
			if (distance > used) {
				throw std::runtime_error("Looking back too many bytes, corrupted archive or insufficient buffer size");
			}
			int toWrite = std::min(distance, length - written);
			memmove(buffer.data() + used, buffer.data() + used - distance, toWrite);
			used += toWrite;
			written += toWrite;
		}
	}

	auto& getChecksum() {
		return checksum;
	}

	void done() { // Called when the whole buffer can be consumed because the data won't be needed anymore
		expectsMore = false;
	}
};

// Represents a table encoding Huffman codewords and can parse the stream by bits
template <int MaxSize, typename ReaderType>
class EncodedTable {
	ReaderType& reader;
	struct CodeIndexEntry {
		int16_t word = 0;
		int8_t length = 0;
		bool valid = false;
	};
	std::array<CodeIndexEntry, 256> codesIndex = {}; // If value is greater than MaxSize, it's a remainder at index value minus MaxSize
	struct CodeRemainder {
		uint8_t remainder = 0;
		uint8_t bitsLeft = 0;
		uint16_t index = 0; // bit or with 0x8000 if it's the last one in sequence
	};
	std::array<CodeRemainder, MaxSize> remainders = {};

public:
	EncodedTable(ReaderType& reader, int realSize, std::array<uint8_t, 256> codeCodingLookup, std::array<uint8_t, codeCodingReorder.size()> codeCodingLengths)
	: reader(reader) {
		std::array<int, 17> quantities = {};
		struct CodeEntry {
			uint8_t start = 0;
			uint8_t ending = 0;
			uint8_t length = 0;
		};
		std::array<CodeEntry, MaxSize> codes = {};

		// Read the Huffman-encoded Huffman codes
		for (int i = 0; i < realSize; ) {
			int length = 0;
			reader.peekAByteAndConsumeSome([&] (uint8_t peeked) {
				length = codeCodingLookup[peeked];
				return codeCodingLengths[length];
			});
			if (length < 16) {
				codes[i].length = length;
				i++;
				quantities[length]++;
			} else if (length == 16) {
				if (i == 0) [[unlikely]]
					throw std::runtime_error("Invalid lookback position");
				int copy = reader.getBits(2) + 3;
				for (int j = i; j < i + copy; j++) {
					codes[j].length = codes[i - 1].length;
				}
				quantities[codes[i - 1].length] += copy;
				i += copy;
			} else if (length > 16) {
				int zeroCount = 0;
				if (length == 17) {
					zeroCount = reader.getBits(3) + 3;
				} else {
					int sevenBitsValue = reader.getBits(7);
					zeroCount = sevenBitsValue + 11;
				};
				for (int j = i; j < i + zeroCount; j++) {
					codes[j].length = 0;
				}
				i += zeroCount;
			}
		}

		struct UnindexedEntry {
			int quantity = 0;
			int startIndex = 0;
			int filled = 0;
		};
		std::array<UnindexedEntry, 256> unindexedEntries = {};

		// Generate the codes
		int nextCode = 0;
		for (int size = 1; size <= 16; size++) {
			if (quantities[size] > 0) {
				for (int i = 0; i <= realSize; i++) {
					if (codes[i].length == size) {
						if (nextCode >= (1 << size)) [[unlikely]]
								throw std::runtime_error("Bad Huffman encoding, run out of Huffman codes");
						uint8_t firstPart = uint8_t(nextCode);
						if (size <= 8) [[likely]] {
							codes[i].start = reversedBytes[firstPart];
							for (int code = codes[i].start >> (8 - size); code < std::ssize(codesIndex); code += (1 << size)) {
								codesIndex[code].word = i;
								codesIndex[code].length = size;
								codesIndex[code].valid = true;
							}
						} else {
							uint8_t start = reversedBytes[uint8_t(nextCode >> (size - 8))];
							codes[i].start = start;
							codesIndex[start].valid = true;
							unindexedEntries[start].quantity++;
							codes[i].ending = reversedBytes[uint8_t(nextCode)] >> (16 - size);
						}
						nextCode++;
					}
				}
			}
			nextCode <<= 1;
		}

		// Calculate ranges of the longer parts
		int currentStartIndex = 0;
		for (auto& entry : unindexedEntries) {
			entry.startIndex = currentStartIndex;
			currentStartIndex += entry.quantity;
		}

		// Index the longer parts
		for (int i = 0; i < std::ssize(codes); i++) {
			CodeEntry& code = codes[i];
			if (code.length > 8) {
				UnindexedEntry& unindexedEntry = unindexedEntries[code.start];
				CodeRemainder& remainder = remainders[unindexedEntry.startIndex + unindexedEntry.filled];
				codesIndex[code.start].word = MaxSize + unindexedEntry.startIndex;
				unindexedEntry.filled++;
				remainder.remainder = code.ending; // The upper bits are cut
				remainder.bitsLeft = code.length - 8;
				remainder.index = i;
				if (unindexedEntry.filled == unindexedEntry.quantity)
					remainder.index |= 0x8000;
			}
		}
	}

	int readWord() {
		int word = 0;
		reader.peekAByteAndConsumeSome([&] (uint8_t peeked) {
			const CodeIndexEntry entry = codesIndex[peeked];
			word = entry.word;
			if (word >= MaxSize) {
				return 8;
			} else if (!entry.valid) {
				throw std::runtime_error("Unknown Huffman code (not even first 8 bits)");
			}
			return int(entry.length);
		});

		// Longer codes than a byte are indexed specially
		static constexpr std::array<uint8_t, 9> endMasks = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
		if (word >= MaxSize) {
			reader.peekAByteAndConsumeSome([&] (uint8_t peeked) {
				for (int i = word - MaxSize; i < MaxSize * 2; i++) {
					if ((peeked & endMasks[remainders[i].bitsLeft]) == remainders[i].remainder) {
						word = remainders[i].index & 0x7fff;
						return remainders[i].bitsLeft;
					}

					if (remainders[i].index & 0x8000) {
						throw std::runtime_error("Unknown Huffman code (ending bits don't fit)");
					}
				}
				throw std::runtime_error("Unknown Huffman code (bad prefix)");
			});
		}
		return word;
	}
};

template <DecompressionSettings Settings>
template <int MaxTableSize>
auto ByteInput<Settings>::encodedTable(int realSize, const std::array<uint8_t, 256>& codeCodingLookup, const std::array<uint8_t, codeCodingReorder.size()>& codeCodingLengths) {
	return EncodedTable<MaxTableSize, ByteInput<Settings>>(*this, realSize, codeCodingLookup, codeCodingLengths);
}

// Higher level class handling the overall state of parsing. Implemented as a state machine to allow pausing when output is full.
template <DecompressionSettings Settings>
class DeflateReader {
	ByteInput<Settings>& input;
	ByteOutput<Settings>& output;

	struct CopyState {
		int copyDistance = 0;
		int copyLength = 0;

		bool restart(ByteOutput<Settings>& output) {
			int copying = std::min(output.available(), copyLength);
			output.repeatSequence(copying, copyDistance);
			copyLength -= copying;
			return (copyLength == 0);
		}
		bool copy(ByteOutput<Settings>& output, int length, int distance) {
			copyLength = length;
			copyDistance = distance;
			return restart(output);
		}
	};

	struct LiteralState {
		int bytesLeft = 0;
		LiteralState(DeflateReader* parent) {
			int length = parent->input.getBytes(2);
			int antiLength = parent->input.getBytes(2);
			if ((~length & 0xffff) != antiLength) {
				throw std::runtime_error("Corrupted data, inverted length of literal block is mismatching");
			}
			bytesLeft = length;
		}

		bool parseSome(DeflateReader* parent) {
			if (parent->output.available() > bytesLeft) {
				std::span<const uint8_t> chunk = parent->input.getRange(bytesLeft);
				parent->output.addBytes(std::span<const char>(reinterpret_cast<const char*>(chunk.data()), (chunk.size())));
				bytesLeft -= chunk.size();
				return (bytesLeft > 0);
			} else {
				std::span<const uint8_t> chunk = parent->input.getRange(parent->output.available());
				bytesLeft -= chunk.size();
				parent->output.addBytes(std::span<const char>(reinterpret_cast<const char*>(chunk.data()), (chunk.size())));
				return true;
			}
		}
	};

	struct FixedCodeState : CopyState {
		BitReader<ByteInput<Settings>> input;
		FixedCodeState(BitReader<ByteInput<Settings>>&& input) : input(std::move(input)) {}

		bool parseSome(DeflateReader* parent) {
			if (CopyState::copyLength > 0) { // Resume copying if necessary
				if (CopyState::restart(parent->output)) {
					return true; // Out of space
				}
			}

			struct CodeEntry {
				int8_t length = 7;
				int code = 0;
			};
			constexpr static std::array<CodeEntry, 256> codeIndex = ArrayFiller([] (uint8_t oneByteReversed) constexpr {
				uint8_t oneByte = reversedBytes[oneByteReversed];
				if (oneByte < 0b00000010) {
					return CodeEntry{7, 256};
				} else if (oneByte < 0b00110000) {
                                        int e = (oneByte >> 1) - 1 + 257; 
                                        assert(e>=0);   
					return CodeEntry{7, (oneByte >> 1) - 1 + 257};
				} else if (oneByte < 0b11000000) {
                                        assert(oneByte >= 0b00110000);
					return CodeEntry{8, oneByte - 0b00110000};
				} else if (oneByte < 0b11001000) {
                                        assert(oneByte >= 0b11000000);
					return CodeEntry{8, oneByte - 0b11000000 + 280};
				} else {
                                        assert(oneByte >= 0b11001000);
					return CodeEntry{8, oneByte - 0b11001000 + 144};
				}
			});

			while (parent->output.available()) {
				CodeEntry code = {};
				input.peekAByteAndConsumeSome([&code] (uint8_t peeked) {
					code = codeIndex[peeked];
					return code.length;
				});

				if (code.code == 256) [[unlikely]] {
					break;
				} else if (code.code > 256) {
					int length = code.code - 254;
					if (length > 10) {
						length = input.parseLongerSize(length);
					}
					static constexpr std::array<uint8_t, 32> lengthDictionary = ArrayFiller([] (int uninverted) constexpr {
						uint8_t reversed = uint8_t((uninverted * uint64_t(0x0202020202) & uint64_t(0x010884422010)) % 0x3ff);
						return uint8_t((reversed >> 3) + 1); // Convert to length word
					});
					int distance = lengthDictionary[input.getBits(5)];
					if (distance > 4) {
						distance = input.parseLongerDistance(distance);
					}
					CopyState::copy(parent->output, length, distance);
				} else {
					if (code.code < 144) {
						parent->output.addByte(code.code);
					} else {
						uint8_t full = ((code.code - 144)) << (1 + 144 + input.getBits(1));
						parent->output.addByte(full);
					}
				}
			}
			return (parent->output.available() == 0);
		}
	};

	struct DynamicCodeState : CopyState {
		BitReader<ByteInput<Settings>> input;
		EncodedTable<288, BitReader<ByteInput<Settings>>> codes;
		EncodedTable<31, BitReader<ByteInput<Settings>>> distanceCode;

		DynamicCodeState(decltype(input)&& inputMoved, int codeCount, int distanceCodeCount, const std::array<uint8_t, 256>& codeCodingLookup,
						const std::array<uint8_t, codeCodingReorder.size()>& codeCodingLengths)
			: input(std::move(inputMoved))
			, codes(input, codeCount, codeCodingLookup, codeCodingLengths)
			, distanceCode(input, distanceCodeCount, codeCodingLookup, codeCodingLengths)
		{ }

		bool parseSome(DeflateReader* parent) {
			if (CopyState::copyLength > 0) { // Resume copying if necessary
				if (CopyState::restart(parent->output)) {
					return true; // Out of space
				}
			}
			while (parent->output.available()) {
				int word = codes.readWord();

				if (word < 256) {
					parent->output.addByte(word);
				} else if (word == 256) [[unlikely]] {
					break;
				} else {
					int length = word - 254;
					if (length > 10) {
						length = input.parseLongerSize(length);
					}
					int distance = distanceCode.readWord() + 1;
					if (distance > 4) {
						distance = input.parseLongerDistance(distance);
					}
					CopyState::copy(parent->output, length, distance);
				}
			}
			return (parent->output.available() == 0);
		}
	};

	std::variant<std::monostate, LiteralState, FixedCodeState, DynamicCodeState> decodingState = {};
	bool wasLast = false;

public:
	DeflateReader(ByteInput<Settings>& input, ByteOutput<Settings>& output) : input(input), output(output) {}

	// Returns whether there is more work to do
	bool parseSome() {
		while (true) {
			BitReader<ByteInput<Settings>> bitInput(nullptr);
			if (LiteralState* state = std::get_if<LiteralState>(&decodingState)) {
				if (state->parseSome(this)) {
					return true;
				}
				bitInput = BitReader<ByteInput<Settings>>(&input);
			} else if (FixedCodeState* state1 = std::get_if<FixedCodeState>(&decodingState)) {
				if (state1->parseSome(this)) {
					return true;
				}
				bitInput = std::move(state1->input);
			} else if (DynamicCodeState* state2 = std::get_if<DynamicCodeState>(&decodingState)) {
				if (state2->parseSome(this)) {
					return true;
				}
				bitInput = std::move(state2->input);
			} else {
				bitInput = BitReader<ByteInput<Settings>>(&input);
			}
			decodingState = std::monostate();

			// No decoding state
			if (wasLast) {
				output.done();
				return false;
			}
			wasLast = bitInput.getBits(1);
			int compressionType = bitInput.getBits(2);
			if (compressionType == 0b00) {
				BitReader(std::move(bitInput)); // Move it to a temporary and destroy it
				decodingState.template emplace<LiteralState>(this);
			} else if (compressionType == 0b01) {
				decodingState.template emplace<FixedCodeState>(std::move(bitInput));
			} else if (compressionType == 0b10) {
				// Read lengths
				const int extraCodes = bitInput.getBits(5); // Will be used later
				constexpr int maximumExtraCodes = 29;
				if (extraCodes > maximumExtraCodes) [[unlikely]] {
					throw std::runtime_error("Impossible number of extra codes");
				}
				const int distanceCodes = bitInput.getBits(5) + 1; // Will be used later
				if (distanceCodes > 31) [[unlikely]] {
					throw std::runtime_error("Impossible number of distance codes");
				}
				const int codeLengthCount = bitInput.getBits(4) + 4;
				if (codeLengthCount > 19) [[unlikely]]
						throw std::runtime_error("Invalid distance code count");

				// Read Huffman code lengths
				std::array<uint8_t, codeCodingReorder.size()> codeCodingLengths = {};
				for (int i = 0; i < codeLengthCount; i++) {
					codeCodingLengths[codeCodingReorder[i]] = static_cast<uint8_t>(bitInput.getBits(3));
				}

				// Generate Huffman codes for lengths
				std::array<int, codeCodingReorder.size()> codeCoding = {};
				std::array<uint8_t, 256> codeCodingLookup = {};
				int nextCodeCoding = 0;
				for (int size = 1; size <= 8; size++) {
					for (int i = 0; i < std::ssize(codeCoding); i++)
						if (codeCodingLengths[i] == size) {

							for (int code = nextCodeCoding << (8 - size); code < (nextCodeCoding + 1) << (8 - size); code++) {
								codeCodingLookup[reversedBytes[code]] = i;
							}
							codeCoding[i] = reversedBytes[nextCodeCoding];

							nextCodeCoding++;
						}
					nextCodeCoding <<= 1;
				}

				decodingState.template emplace<DynamicCodeState>(std::move(bitInput), 257 + extraCodes, distanceCodes, codeCodingLookup, codeCodingLengths);
			} else {
				throw std::runtime_error("Unknown type of block compression");
			}
		}
	}
};

} // namespace Detail

// Handles decompression of a deflate-compressed archive, no headers
template <DecompressionSettings Settings = DefaultDecompressionSettings>
std::vector<char> readDeflateIntoVector(std::function<int(std::span<uint8_t> batch)> readMoreFunction) {
	std::vector<char> result;
	Detail::ByteInput<Settings> input(readMoreFunction);
	Detail::ByteOutput<Settings> output;
	Detail::DeflateReader reader(input, output);
	bool workToDo = false;
	do {
		workToDo = reader.parseSome();
		std::span<const char> batch = output.consume();
		result.insert(result.end(), batch.begin(), batch.end());
	} while (workToDo);
	return result;
}

template <DecompressionSettings Settings = DefaultDecompressionSettings>
std::vector<char> readDeflateIntoVector(std::span<const uint8_t> allData) {
	return readDeflateIntoVector<Settings>([allData, position = 0] (std::span<uint8_t> toFill) mutable -> int {
		int filling = std::min(allData.size() - position, toFill.size());
		if(filling != 0)
			memcpy(toFill.data(), &allData[position], filling);
		position += filling;
		return filling;
	});
}

// Handles decompression of a deflate-compressed archive, no headers
template <DecompressionSettings Settings = DefaultDecompressionSettings>
class IDeflateArchive {
protected:
	Detail::ByteInput<Settings> input;
	Detail::ByteOutput<Settings> output;
	Detail::DeflateReader<Settings> deflateReader = {input, output};
	bool done = false;

	virtual void onFinish() {}

public:
	IDeflateArchive(std::function<int(std::span<uint8_t> batch)> readMoreFunction) : input(readMoreFunction) {}

#ifndef EZGZ_NO_FILE
	IDeflateArchive(const std::string& fileName) : input([file = std::make_shared<std::ifstream>(fileName, std::ios::binary)] (std::span<uint8_t> batch) mutable {
		if (!file->good()) {
			throw std::runtime_error("Can't read file");
		}
		file->read(reinterpret_cast<char*>(batch.data()), batch.size());
		int bytesRead = static_cast<int>(file->gcount());
		if (bytesRead == 0) {
			throw std::runtime_error("Truncated file");
		}
		return bytesRead;
	}) {}
#endif

	IDeflateArchive(std::span<const uint8_t> data) : input([data] (std::span<uint8_t> batch) mutable {
		int copying = std::min(batch.size(), data.size());
		if (copying == 0) {
			throw std::runtime_error("Truncated input");
		}
		memcpy(batch.data(), data.data(), copying);
		data = std::span<const uint8_t>(data.begin() + copying, data.end());
		return copying;
	}) {}

	// Returns whether there are more bytes to read
	std::optional<std::span<const char>> readSome(int bytesToKeep = 0) {
		if (done) {
			return std::nullopt;
		}
		bool moreStuffToDo = deflateReader.parseSome();
		std::span<const char> batch = output.consume(bytesToKeep);
		if (!moreStuffToDo) {
			onFinish();
			done = true;
		}
		return batch;
	}

	void readByLines(const std::function<void(std::span<const char>)> reader, char separator = '\n') {
		int keeping = 0;
		std::span<const char> batch = {};
		bool wasSeparator = false;
		while (std::optional<std::span<const char>> batchOrNot = readSome(keeping)) {
			batch = *batchOrNot;
			std::span<const char>::iterator start = batch.begin();
			for (std::span<const char>::iterator it = start; it != batch.end(); ++it) {
				if (wasSeparator) {
					wasSeparator = false;
					start = it;
				}
				if (*it == separator) {
					reader(std::span<const char>(start, it));
					wasSeparator = true;
				}
			}
			keeping = batch.end() - start;
		}
		if (keeping > 0) {
			if (wasSeparator)
				reader(std::span<const char>());
			else
				reader(std::span<const char>(batch.end() - keeping, batch.end()));
		}
	}

	void readAll(const std::function<void(std::span<const char>)>& reader) {
		while (std::optional<std::span<const char>> batch = readSome()) {
			reader(*batch);
		}
	}

	std::vector<char> readAll() {
		std::vector<char> returned;
		while (std::optional<std::span<const char>> batch = readSome()) {
			returned.insert(returned.end(), batch->begin(), batch->end());
		};
		return returned;
	}
};

enum class CreatingOperatingSystem {
	UNIX_BASED,
	WINDOWS,
	OTHER
};

// File information in the .gz file
template <BasicStringType StringType>
struct IGzFileInfo {
	int32_t modificationTime = 0;
	CreatingOperatingSystem operatingSystem = CreatingOperatingSystem::OTHER;
	bool fastestCompression = false;
	bool densestCompression = false;
	std::optional<std::vector<uint8_t>> extraData = {};
	StringType name;
	StringType comment;
	bool probablyText = false;

	template <DecompressionSettings Settings>
	IGzFileInfo(Detail::ByteInput<Settings>& input) {
		typename Settings::Checksum checksum = {};
		auto check = [&checksum] (auto num) -> uint32_t {
			std::array<uint8_t, sizeof(num)> asBytes = {};
			memcpy(asBytes.data(), &num, asBytes.size());
			return checksum(asBytes);
		};
		if (input.template getInteger<uint8_t>() != 0x1f || input.template getInteger<uint8_t>() != 0x8b || input.template getInteger<uint8_t>() != 0x08)
			throw std::runtime_error("Trying to parse something that isn't a Gzip archive");
		check(0x1f);
		check(0x8b);
		check(0x08);
		uint8_t flags = static_cast<uint8_t>(input.template getInteger<uint8_t>());
		check(flags);
		modificationTime = static_cast<int32_t>(input.template getInteger<uint32_t>());
		check(modificationTime);
		uint8_t extraFlags = static_cast<uint8_t>(input.template getInteger<uint8_t>());
		check(extraFlags);

		if (extraFlags == 4) {
			densestCompression = true;
		} else if (extraFlags == 8) {
			fastestCompression = true;
		}
		uint8_t creatingOperatingSystem = static_cast<uint8_t>(input.template getInteger<uint8_t>()); // Was at input[9]
		check(creatingOperatingSystem);
		if (creatingOperatingSystem == 0) {
			operatingSystem = CreatingOperatingSystem::WINDOWS;
		} else if (creatingOperatingSystem == 3) {
			operatingSystem = CreatingOperatingSystem::UNIX_BASED;
		}

		if (flags & 0x04) {
			uint16_t extraHeaderSize = static_cast<uint16_t>(input.template getInteger<uint16_t>());
			check(extraHeaderSize);
			int readSoFar = 0;
			extraData.emplace();
			while (readSoFar < extraHeaderSize) {
				std::span<const uint8_t> taken = input.getRange(extraHeaderSize - readSoFar);
				checksum(taken);
				extraData->insert(extraData->end(), taken.begin(), taken.end());
				readSoFar += taken.size();
			}
		}
		if (flags & 0x08) {
			char letter = static_cast<char>(input.template getInteger<uint8_t>());
			check(letter);
			while (letter != '\0') {
				name += letter;
				letter = input.template getInteger<uint8_t>();
				check(letter);
			}
		}
		if (flags & 0x10) {
			char letter = input.template getInteger<uint8_t>();
			check(letter);
			while (letter != '\0') {
				comment += letter;
				letter = input.template getInteger<uint8_t>();
				check(letter);
			}
		}
		if (flags & 0x01) {
			probablyText = true;
		}
		if (flags & 0x02) {
			uint16_t expectedHeaderCrc = input.template getInteger<uint16_t>();
			check(expectedHeaderCrc);
			uint16_t realHeaderCrc = checksum();
			if (expectedHeaderCrc != realHeaderCrc)
				throw std::runtime_error("Gzip archive's headers crc32 checksum doesn't match the actual header's checksum");
		}
	}
};

// Parses a .gz file, only takes care of the header, the rest is handled by its parent class IDeflateArchive
template <DecompressionSettings Settings = DefaultDecompressionSettings>
class IGzFile : public IDeflateArchive<Settings> {
	IGzFileInfo<typename Settings::StringType> parsedHeader;
	using Deflate = IDeflateArchive<Settings>;

	void onFinish() override {
		uint32_t expectedCrc = static_cast<uint32_t>(Deflate::input.template getInteger<uint32_t>());
		if constexpr(Settings::verifyChecksum) {
			auto realCrc = Deflate::output.getChecksum()();
			if (expectedCrc != realCrc)
				throw std::runtime_error("Gzip archive's crc32 checksum doesn't match the calculated checksum");
		}
	}

public:
	IGzFile(std::function<int(std::span<uint8_t> batch)> readMoreFunction) : Deflate(readMoreFunction), parsedHeader(Deflate::input) {}
	IGzFile(const std::string& fileName) : Deflate(fileName), parsedHeader(Deflate::input) {}
	IGzFile(std::span<const uint8_t> data) : Deflate(data), parsedHeader(Deflate::input) {}

	const IGzFileInfo<typename Settings::StringType>& info() const {
		return parsedHeader;
	}
};

namespace Detail {
template <DecompressionSettings Settings = DefaultDecompressionSettings>
class IGzStreamBuffer : public std::streambuf {
	IGzFile<Settings> inputFile;
	int bytesToKeep = 10;
	ptrdiff_t produced = 0;
public:
	template<typename Arg>
	IGzStreamBuffer(const Arg& arg, int bytesToKeep) : inputFile(arg), bytesToKeep(bytesToKeep) {}

	int underflow() override {
		std::optional<std::span<const char>> batch = inputFile.readSome(bytesToKeep);
		if (batch.has_value()) {
			// We have to believe std::istream that it won't edit the data, otherwise it would be necessary to copy the data
			char* start = const_cast<char*>(batch->data());
			setg(start - std::min<ptrdiff_t>(bytesToKeep, produced), start, start + batch->size());
			produced += batch->size();
			return traits_type::to_int_type(*gptr());
		} else {
			return traits_type::eof();
		}
	}

	const IGzFileInfo<typename Settings::StringType>& info() const {
		return inputFile.info();
	}
};
}

// Using IGzFile as std::istream, configurable
template <DecompressionSettings Settings = DefaultDecompressionSettings>
class BasicIGzStream : private Detail::IGzStreamBuffer<Settings>, public std::istream
{
public:
#ifndef EZGZ_NO_FILE
	// Open and read a file, always keeping the given number of characters specified in the second character (10 by default)
	BasicIGzStream(const std::string& sourceFile, int bytesToKeep = 10) : Detail::IGzStreamBuffer<Settings>(sourceFile, bytesToKeep), std::istream(this) {}
#endif
	// Read from a buffer
	BasicIGzStream(std::span<const uint8_t> data, int bytesToKeep = 10) : Detail::IGzStreamBuffer<Settings>(data, bytesToKeep),  std::istream(this) {}
	// Use a function that fills a buffer of data and returns how many bytes it wrote
	BasicIGzStream(std::function<int(std::span<uint8_t> batch)> readMoreFunction, int bytesToKeep = 10) : Detail::IGzStreamBuffer<Settings>(readMoreFunction, bytesToKeep), std::istream(this) {}
	// Read from an existing stream
	BasicIGzStream(std::istream& input, int bytesToKeep = 10) : Detail::IGzStreamBuffer<Settings>([&input] (std::span<uint8_t> batch) -> int {
		input.read(reinterpret_cast<char*>(batch.data()), batch.size());
		return input.gcount();
	}, bytesToKeep), std::istream(this) {}

	using Detail::IGzStreamBuffer<Settings>::info;
};

// Most obvious usage, default settings
using IGzStream = BasicIGzStream<>;

} // namespace EzGz

#endif // EZGZ_HPP
