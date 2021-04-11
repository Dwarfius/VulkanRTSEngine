#include "Precomp.h"
#include "Utils.h"

namespace Utils
{
	bool IsNan(glm::vec3 aVec)
	{
		return glm::isnan(aVec) == glm::vec3::bool_type(true);
	}

	bool IsNan(glm::vec4 aVec)
	{
		return glm::isnan(aVec) == glm::vec4::bool_type(true);
	}

	// taken from https://en.wikipedia.org/wiki/Base64#Base64_table
	constexpr static char kBase64EncodeTable[64] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/'
	};
	constexpr char kBase64Padding = '=';

	struct Base64DecodeTable
	{
		constexpr static char kBeginIndex = '+';
		constexpr static char kEndIndex = '{';
		char decodeTable[kEndIndex - kBeginIndex];

		constexpr Base64DecodeTable()
			: decodeTable()
		{
			// default init all to 0
			for (size_t i = kBeginIndex; i < kEndIndex - kBeginIndex; i++)
			{
				decodeTable[i] = 0;
			}
			// invert the table
			for (char i = 0; i < 64; i++)
			{
				decodeTable[kBase64EncodeTable[i] - kBeginIndex] = i;
			}
		}

		constexpr char Decode(char aSextet) const
		{
			ASSERT(aSextet >= kBeginIndex && aSextet < kEndIndex);
			return decodeTable[aSextet - kBeginIndex];
		}
	};
	constexpr static Base64DecodeTable kBase64DecodeTable;

	std::vector<char> Base64Encode(const std::vector<char>& anInput)
	{
		std::vector<char> encodedBuffer;
		const size_t triOctetCountWithPadding = static_cast<size_t>(glm::ceil(anInput.size() / 3.f));
		const size_t expectedSize = 4 * triOctetCountWithPadding;
		encodedBuffer.resize(expectedSize);
		// Base64 is a six-bit encoding scheme, so:
		// read 6 bits -> record value from table -> move to next 6
		constexpr char kLeftOctetMasks[4] = {
			static_cast<char>(0b11111100), // index 0 
			static_cast<char>(0b00000011), // index 0
			static_cast<char>(0b00001111), // index 1
			static_cast<char>(0b00111111)  // index 2
		};
		constexpr char kRightOctetMasks[4] = {
			static_cast<char>(0b00000000), // index 1
			static_cast<char>(0b11110000), // index 1
			static_cast<char>(0b11000000), // index 2
			static_cast<char>(0b00000000)  // index 3
		};
		const size_t triOctetCount = anInput.size() / 3;
		// TODO: play around with simd intrinsics
		for (size_t i = 0; i < triOctetCount; i++)
		{
			char octet1 = anInput[i * 3];
			char octet2 = anInput[i * 3 + 1];
			char octet3 = anInput[i * 3 + 2];

			char sextet1 = (octet1 & kLeftOctetMasks[0]) >> 2;
			char sextet2 = ((octet1 & kLeftOctetMasks[1]) << 4) |
							((octet2 & kRightOctetMasks[1]) >> 4);
			char sextet3 = ((octet2 & kLeftOctetMasks[2]) << 2) |
							((octet3 & kRightOctetMasks[2]) >> 6);
			char sextet4 = octet3 & kLeftOctetMasks[3];

			encodedBuffer[i * 4] = kBase64EncodeTable[sextet1];
			encodedBuffer[i * 4 + 1] = kBase64EncodeTable[sextet2];
			encodedBuffer[i * 4 + 2] = kBase64EncodeTable[sextet3];
			encodedBuffer[i * 4 + 3] = kBase64EncodeTable[sextet4];
		}
		// at the end, if there's not enough source data to fill
		// encoded buffer, add padding symbols
		for (size_t i = triOctetCount; i < triOctetCountWithPadding; i++)
		{
			bool octet2Valid = (i * 3 + 1) < anInput.size();
			bool octet3Valid = (i * 3 + 2) < anInput.size();

			char octet1 = anInput[i * 3];
			char octet2 = octet2Valid ? anInput[i * 3 + 1] : 0;
			char octet3 = octet3Valid ? anInput[i * 3 + 2] : 0;

			char sextet1 = (octet1 & kLeftOctetMasks[0]) >> 2;
			char sextet2 = ((octet1 & kLeftOctetMasks[1]) << 4) |
				((octet2 & kRightOctetMasks[1]) >> 4);
			char sextet3 = ((octet2 & kLeftOctetMasks[2]) << 2) |
				((octet3 & kRightOctetMasks[2]) >> 6);
			char sextet4 = octet3 & kLeftOctetMasks[3];

			encodedBuffer[i * 4] = kBase64EncodeTable[sextet1];
			encodedBuffer[i * 4 + 1] = kBase64EncodeTable[sextet2];
			encodedBuffer[i * 4 + 2] = octet2Valid ? kBase64EncodeTable[sextet3] : kBase64Padding;
			encodedBuffer[i * 4 + 3] = octet3Valid ? kBase64EncodeTable[sextet4] : kBase64Padding;
		}
		return encodedBuffer;
	}

	template<class TInput>
	std::vector<char> Base64Decode_Impl(TInput&& anInput)
	{
		std::vector<char> decodedBuffer;
		const size_t quadSextetCount = static_cast<size_t>(glm::ceil(anInput.size() / 4.f));
		const size_t expectedSize = 3 * quadSextetCount;
		decodedBuffer.resize(expectedSize); // with padding!
		constexpr char kLeftSextetMasks[3] = {
			0b00111111, // index 0 
			0b00001111, // index 1
			0b00000011  // index 2
		};
		constexpr char kRightSextetMasks[3] = {
			0b00110000, // index 1
			0b00111100, // index 2
			0b00111111  // index 3
		};
		// TODO: play around with simd intrinsics
		for (size_t i = 0; i < quadSextetCount; i++)
		{
			bool sextet3Valid = (i * 4 + 2) < anInput.size();
			bool sextet4Valid = (i * 4 + 3) < anInput.size();

			char sextet1 = anInput[i * 4];
			char sextet2 = anInput[i * 4 + 1];
			char sextet3 = sextet3Valid ? anInput[i * 4 + 2] : '='; // = will default to 0 on decode
			char sextet4 = sextet4Valid ? anInput[i * 4 + 3] : '=';

			// decode the individual sextets
			sextet1 = kBase64DecodeTable.Decode(sextet1);
			sextet2 = kBase64DecodeTable.Decode(sextet2);
			sextet3 = kBase64DecodeTable.Decode(sextet3);
			sextet4 = kBase64DecodeTable.Decode(sextet4);

			char octet1 = (sextet1 << 2) | ((sextet2 & kRightSextetMasks[0]) >> 4);
			char octet2 = ((sextet2 & kLeftSextetMasks[1]) << 4)
				| ((sextet3 & kRightSextetMasks[1]) >> 2);
			char octet3 = ((sextet3 & kLeftSextetMasks[2]) << 6) | sextet4;

			decodedBuffer[i * 3] = octet1;
			decodedBuffer[i * 3 + 1] = octet2;
			decodedBuffer[i * 3 + 2] = octet3;
		}

		// we've decoded the buffer, but we left in decoded paddings, 
		// and they're not part of the data, so removing them
		if (anInput[anInput.size() - 2] == '=')
		{
			decodedBuffer.pop_back();
			decodedBuffer.pop_back();
		}
		else if (anInput[anInput.size() - 1] == '=')
		{
			decodedBuffer.pop_back();
		}

		return decodedBuffer;
	}

	std::vector<char> Base64Decode(const std::vector<char>& anInput)
	{
		return Base64Decode_Impl(anInput);
	}

	std::vector<char> Base64Decode(std::string_view anInput)
	{
		return Base64Decode_Impl(anInput);
	}
}