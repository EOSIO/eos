#pragma once

#include "Platform/Platform.h"

#include "../../../chain/include/eosio/chain/wasm_eosio_constraints.hpp"
#include <string>
#include <vector>
#include <string.h>
#include <algorithm>

namespace Serialization
{
	// An exception that is thrown for various errors during serialization.
	// Any code using serialization should handle it!
	struct FatalSerializationException
	{
		std::string message;
		FatalSerializationException(std::string&& inMessage)
		: message(std::move(inMessage)) {}
	};

	// An abstract output stream.
	struct OutputStream
	{
		enum { isInput = false };

		OutputStream(): next(nullptr), end(nullptr) {}

		Uptr capacity() const { return SIZE_MAX; }
		
		// Advances the stream cursor by numBytes, and returns a pointer to the previous stream cursor.
		inline U8* advance(Uptr numBytes)
		{
			if(next + numBytes > end) { extendBuffer(numBytes); }
			WAVM_ASSERT_THROW(next + numBytes <= end);

			U8* data = next;
			next += numBytes;
			return data;
		}

	protected:

		U8* next;
		U8* end;

		// Called when there isn't enough space in the buffer to hold a write to the stream.
		// Should update next and end to point to a new buffer, and ensure that the new
		// buffer has at least numBytes. May throw FatalSerializationException.
		virtual void extendBuffer(Uptr numBytes) = 0;
	};

	// An output stream that writes to an array of bytes.
	struct ArrayOutputStream : public OutputStream
	{
		// Moves the output array from the stream to the caller.
		std::vector<U8>&& getBytes()
		{
			bytes.resize(next - bytes.data());
			next = nullptr;
			end = nullptr;
			return std::move(bytes);
		}
		
	private:

		std::vector<U8> bytes;

		virtual void extendBuffer(Uptr numBytes)
		{
			const Uptr nextIndex = next - bytes.data();

			// Grow the array by larger and larger increments, so the time spent growing
			// the buffer is O(1).
			bytes.resize(std::max((Uptr)nextIndex+numBytes,(Uptr)bytes.size() * 7 / 5 + 32));

			next = bytes.data() + nextIndex;
			end = bytes.data() + bytes.size();
		}
		virtual bool canExtendBuffer(Uptr numBytes) const { return true; }
	};

	// An abstract input stream.
	struct InputStream
	{
		enum { isInput = true };

		InputStream(const U8* inNext,const U8* inEnd): next(inNext), end(inEnd) {}

		virtual Uptr capacity() const = 0;
		
		// Advances the stream cursor by numBytes, and returns a pointer to the previous stream cursor.
		inline const U8* advance(Uptr numBytes)
		{
			if(next + numBytes > end) { getMoreData(numBytes); }
			const U8* data = next;
			next += numBytes;
			return data;
		}

		// Returns a pointer to the current stream cursor, ensuring that there are at least numBytes following it.
		inline const U8* peek(Uptr numBytes)
		{
			if(next + numBytes > end) { getMoreData(numBytes); }
			return next;
		}

	protected:

		const U8* next;
		const U8* end;
		
		// Called when there isn't enough space in the buffer to satisfy a read from the stream.
		// Should update next and end to point to a new buffer, and ensure that the new
		// buffer has at least numBytes. May throw FatalSerializationException.
		virtual void getMoreData(Uptr numBytes) = 0;
	};

	// An input stream that reads from a contiguous range of memory.
	struct MemoryInputStream : InputStream
	{
		MemoryInputStream(const U8* begin,Uptr numBytes): InputStream(begin,begin+numBytes) {}
		virtual Uptr capacity() const { return end - next; }
	private:
		virtual void getMoreData(Uptr numBytes) { throw FatalSerializationException("expected data but found end of stream"); }
	};

	// Serialize raw byte sequences.
	FORCEINLINE void serializeBytes(OutputStream& stream,const U8* bytes,Uptr numBytes)
	{ memcpy(stream.advance(numBytes),bytes,numBytes); }
	FORCEINLINE void serializeBytes(InputStream& stream,U8* bytes,Uptr numBytes)
	{ 
      if ( numBytes < eosio::chain::wasm_constraints::wasm_page_size )
         memcpy(bytes,stream.advance(numBytes),numBytes); 
      else
         throw FatalSerializationException(std::string("Trying to deserialize bytes of size : " + std::to_string((uint64_t)numBytes)));
   }
	
	// Serialize basic C++ types.
	template<typename Stream,typename Value>
	FORCEINLINE void serializeNativeValue(Stream& stream,Value& value) { serializeBytes(stream,(U8*)&value,sizeof(Value)); }

	template<typename Stream> void serialize(Stream& stream,U8& i) { serializeNativeValue(stream,i); }
	template<typename Stream> void serialize(Stream& stream,U32& i) { serializeNativeValue(stream,i); }
	template<typename Stream> void serialize(Stream& stream,U64& i) { serializeNativeValue(stream,i);  }
	template<typename Stream> void serialize(Stream& stream,I8& i) { serializeNativeValue(stream,i); }
	template<typename Stream> void serialize(Stream& stream,I32& i) { serializeNativeValue(stream,i); }
	template<typename Stream> void serialize(Stream& stream,I64& i) { serializeNativeValue(stream,i); }
	template<typename Stream> void serialize(Stream& stream,F32& f) { serializeNativeValue(stream,f); }
	template<typename Stream> void serialize(Stream& stream,F64& f) { serializeNativeValue(stream,f); }

	// LEB128 variable-length integer serialization.
	template<typename Value,Uptr maxBits>
	FORCEINLINE void serializeVarInt(OutputStream& stream,Value& inValue,Value minValue,Value maxValue)
	{
		Value value = inValue;

		if(value < minValue || value > maxValue)
		{
			throw FatalSerializationException(std::string("out-of-range value: ") + std::to_string(minValue) + "<=" + std::to_string(value) + "<=" + std::to_string(maxValue));
		}

		bool more = true;
		while(more)
		{
			U8 outputByte = (U8)(value&127);
			value >>= 7;
			more = std::is_signed<Value>::value
				? (value != 0 && value != Value(-1)) || (value >= 0 && (outputByte & 0x40)) || (value < 0 && !(outputByte & 0x40))
				: (value != 0);
			if(more) { outputByte |= 0x80; }
			*stream.advance(1) = outputByte;
		};
	}
	
	template<typename Value,Uptr maxBits>
	FORCEINLINE void serializeVarInt(InputStream& stream,Value& value,Value minValue,Value maxValue)
	{
		// First, read the variable number of input bytes into a fixed size buffer.
		enum { maxBytes = (maxBits + 6) / 7 };
		U8 bytes[maxBytes] = {0};
		Uptr numBytes = 0;
		I8 signExtendShift = (I8)sizeof(Value) * 8;
		while(numBytes < maxBytes)
		{
			U8 byte = *stream.advance(1);
			bytes[numBytes] = byte;
			++numBytes;
			signExtendShift -= 7;
			if(!(byte & 0x80)) { break; }
		};

		// Ensure that the input does not encode more than maxBits of data.
		enum { numUsedBitsInLastByte = maxBits - (maxBytes-1) * 7 };
		enum { numUnusedBitsInLast = 8 - numUsedBitsInLastByte };
		enum { lastBitUsedMask = U8(1<<(numUsedBitsInLastByte-1)) };
		enum { lastByteUsedMask = U8(1<<numUsedBitsInLastByte)-U8(1) };
		enum { lastByteSignedMask = U8(~U8(lastByteUsedMask) & ~U8(0x80)) };
		const U8 lastByte = bytes[maxBytes-1];
		if(!std::is_signed<Value>::value)
		{
			if((lastByte & ~lastByteUsedMask) != 0)
			{
				throw FatalSerializationException("Invalid unsigned LEB encoding: unused bits in final byte must be 0");
			}
		}
		else
		{
			const I8 signBit = I8((lastByte & lastBitUsedMask) << numUnusedBitsInLast);
			const I8 signExtendedLastBit = signBit >> numUnusedBitsInLast;
			if((lastByte & ~lastByteUsedMask) != (signExtendedLastBit & lastByteSignedMask))
			{
				throw FatalSerializationException(
					"Invalid signed LEB encoding: unused bits in final byte must match the most-significant used bit");
			}
		}

		// Decode the buffer's bytes into the output integer.
		value = 0;
		for(Uptr byteIndex = 0;byteIndex < maxBytes;++byteIndex)
		{ value |= Value(bytes[byteIndex] & ~0x80) << (byteIndex * 7); }
		
		// Sign extend the output integer to the full size of Value.
		if(std::is_signed<Value>::value && signExtendShift > 0)
		{ value = Value(value << signExtendShift) >> signExtendShift; }

		// Check that the output integer is in the expected range.
		if(value < minValue || value > maxValue)
		{ throw FatalSerializationException(std::string("out-of-range value: ") + std::to_string(minValue) + "<=" + std::to_string(value) + "<=" + std::to_string(maxValue)); }
	}

	// Helpers for various common LEB128 parameters.
	template<typename Stream,typename Value> void serializeVarUInt1(Stream& stream,Value& value) { serializeVarInt<Value,1>(stream,value,0,1); }
	template<typename Stream,typename Value> void serializeVarUInt7(Stream& stream,Value& value) { serializeVarInt<Value,7>(stream,value,0,127); }
	template<typename Stream,typename Value> void serializeVarUInt32(Stream& stream,Value& value) { serializeVarInt<Value,32>(stream,value,0,UINT32_MAX); }
	template<typename Stream,typename Value> void serializeVarUInt64(Stream& stream,Value& value) { serializeVarInt<Value,64>(stream,value,0,UINT64_MAX); }
	template<typename Stream,typename Value> void serializeVarInt7(Stream& stream,Value& value) { serializeVarInt<Value,7>(stream,value,-64,63); }
	template<typename Stream,typename Value> void serializeVarInt32(Stream& stream,Value& value) { serializeVarInt<Value,32>(stream,value,INT32_MIN,INT32_MAX); }
	template<typename Stream,typename Value> void serializeVarInt64(Stream& stream,Value& value) { serializeVarInt<Value,64>(stream,value,INT64_MIN,INT64_MAX); }

	// Serializes a constant. If deserializing, throws a FatalSerializationException if the deserialized value doesn't match the constant.
	template<typename Constant>
	void serializeConstant(InputStream& stream,const char* constantMismatchMessage,Constant constant)
	{
		Constant savedConstant;
		serialize(stream,savedConstant);
		if(savedConstant != constant)
		{
			throw FatalSerializationException(std::string(constantMismatchMessage) + ": loaded " + std::to_string(savedConstant) + " but was expecting " + std::to_string(constant));
		}
	}
	template<typename Constant>
	void serializeConstant(OutputStream& stream,const char* constantMismatchMessage,Constant constant)
	{
		serialize(stream,constant);
	}

	// Serialize containers.
	template<typename Stream>
	void serialize(Stream& stream,std::string& string)
	{
      constexpr size_t max_size = eosio::chain::wasm_constraints::maximum_func_local_bytes;
		Uptr size = string.size();
		serializeVarUInt32(stream,size);
		if(Stream::isInput)
		{
			// Advance the stream before resizing the string:
			// try to get a serialization exception before making a huge allocation for malformed input.
			const U8* inputBytes = stream.advance(size);
         if (size >= max_size)
            throw FatalSerializationException(std::string("Trying to deserialize string of size : " + std::to_string((uint64_t)size) + ", which is over by "+std::to_string(size - max_size )+" bytes"));
			string.resize(size);
			memcpy(const_cast<char*>(string.data()),inputBytes,size);
			string.shrink_to_fit();
		}
		else { serializeBytes(stream,(U8*)string.c_str(),size); }
	}

	template<typename Stream,typename Element,typename Allocator,typename SerializeElement>
	void serializeArray(Stream& stream,std::vector<Element,Allocator>& vector,SerializeElement serializeElement)
	{
      constexpr size_t max_size = eosio::chain::wasm_constraints::maximum_func_local_bytes;
		Uptr size = vector.size();
		serializeVarUInt32(stream,size);
		if(Stream::isInput)
		{
			// Grow the vector one element at a time:
			// try to get a serialization exception before making a huge allocation for malformed input.
			vector.clear();
         if (size >= max_size)
            throw FatalSerializationException(std::string("Trying to deserialize array of size : " + std::to_string((uint64_t)size) + ", which is over by "+std::to_string(size - max_size )+" bytes"));
			for(Uptr index = 0;index < size;++index)
			{
				vector.push_back(Element());
				serializeElement(stream,vector.back());
			}
         vector.shrink_to_fit();
		}
		else
		{
			for(Uptr index = 0;index < vector.size();++index) { serializeElement(stream,vector[index]); }
		}
	}

	template<typename Stream,typename Element,typename Allocator>
	void serialize(Stream& stream,std::vector<Element,Allocator>& vector)
	{
		serializeArray(stream,vector,[](Stream& stream,Element& element){serialize(stream,element);});
	}
}
