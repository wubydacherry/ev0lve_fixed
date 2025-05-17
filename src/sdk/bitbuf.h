#ifndef SDK_BITBUF_H
#define SDK_BITBUF_H

namespace sdk
{
inline uint32_t bit_write_masks[32][33]{};
inline uint32_t bit_extra_masks[32]{};

__forceinline uint32_t bit_for_bitnum(uint32_t n)
{
	static uint32_t bits_for_bitnum[] = {
		(1 << 0),  (1 << 1),  (1 << 2),	 (1 << 3),	(1 << 4),  (1 << 5),  (1 << 6),	 (1 << 7),
		(1 << 8),  (1 << 9),  (1 << 10), (1 << 11), (1 << 12), (1 << 13), (1 << 14), (1 << 15),
		(1 << 16), (1 << 17), (1 << 18), (1 << 19), (1 << 20), (1 << 21), (1 << 22), (1 << 23),
		(1 << 24), (1 << 25), (1 << 26), (1 << 27), (1 << 28), (1 << 29), (1 << 30), static_cast<uint32_t>((1 << 31)),
	};

	return bits_for_bitnum[n & 31];
}

inline void init_masks()
{
	for (uint32_t sb{}; sb < 32; ++sb)
	{
		for (uint32_t bl{}; bl < 33; ++bl)
		{
			const auto end_bit = sb + bl;
			bit_write_masks[sb][bl] = bit_for_bitnum(sb) - 1;
			if (end_bit < 32)
				bit_write_masks[sb][bl] |= ~(bit_for_bitnum(end_bit) - 1);
		}
	}

	for (uint32_t mb{}; mb < 32; ++mb)
		bit_extra_masks[mb] = bit_for_bitnum(mb) - 1;
}

inline constexpr auto coord_integer_bits = 14;
inline constexpr auto coord_fractional_bits = 5;
inline constexpr auto coord_denominator = 1 << coord_fractional_bits;
inline constexpr auto coord_resolution = 1.0 / coord_denominator;

class bf_write
{
public:
	__forceinline bf_write(char *data, size_t size) : data(data), data_size(size & ~3), cur_bit(0)
	{
		data_bits = (size & ~XOR_32(3)) << XOR_32(3);
	}

	__forceinline void write_bit(bool v)
	{
		const auto bitval = (1 << (cur_bit & XOR_32(7)));
		const auto write_pad = cur_bit >> XOR_32(3);

		if (v)
			((uint32_t *)data)[write_pad] |= bitval;
		else
			((uint32_t *)data)[write_pad] &= ~bitval;

		++cur_bit;
	}

	__forceinline void write_ulong(uint32_t d, uint32_t bits)
	{
		// can't write :(
		if (cur_bit + bits > data_bits)
			return;

		const auto cur_byte = cur_bit >> XOR_32(5);
		const auto cur_masked = cur_bit & XOR_32(31);

		// write initially
		auto cur = ((uint32_t *)data)[cur_byte];
		cur &= bit_write_masks[cur_masked][bits];
		cur |= d << cur_masked;

		((uint32_t *)data)[cur_byte] = cur;

		// check if cant fit
		const auto written = XOR_32(32) - cur_masked;
		if (written < bits)
		{
			bits -= written;
			d >>= written;

			// write onto another to keep it nice and tidy
			cur = ((uint32_t *)data)[cur_byte + 1];
			cur &= bit_write_masks[0][bits];
			cur |= d;

			((uint32_t *)data)[cur_byte + 1] = cur;
		}

		cur_bit += bits;
	}

	__forceinline void write_coord(float f)
	{
		const auto sign_bit = (f <= -coord_resolution);
		const auto fract_val = abs((int)(f * coord_denominator) & (coord_denominator - 1));
		auto int_val = (int)abs(f);

		write_bit(!!int_val);
		write_bit(!!fract_val);

		if (int_val || fract_val)
		{
			write_bit(!!sign_bit);

			if (int_val)
			{
				--int_val;
				write_ulong((uint32_t)int_val, coord_integer_bits);
			}

			if (fract_val)
				write_ulong((uint32_t)fract_val, coord_fractional_bits);
		}
	}

	__forceinline void write_byte(uint8_t v) { write_ulong(v, sizeof(v) << XOR_32(3)); }
	__forceinline void write_word(uint16_t v) { write_ulong(v, sizeof(v) << XOR_32(3)); }
	__forceinline void write_dword(uint32_t v) { write_ulong(v, sizeof(v) << XOR_32(3)); }

private:
	char *data;
	size_t data_size;
	size_t data_bits;
	size_t cur_bit;
};

class bf_read
{
public:
	__forceinline bf_read(char *data, size_t size) : data(data), data_size(size & ~3), cur_bit(0)
	{
		data_bits = (size & ~XOR_32(3)) << XOR_32(3);
	}

	__forceinline bool read_bit()
	{
		const auto v = !!(((uint32_t *)data)[cur_bit >> XOR_32(3)] & (1 << (cur_bit & XOR_32(7))));
		++cur_bit;
		return v;
	}

	__forceinline uint32_t read_ulong(uint32_t bits)
	{
		// boohoo
		if (cur_bit + bits > data_bits)
			return 0;

		const auto cur_byte = cur_bit >> XOR_32(5);
		auto val = ((uint32_t *)data)[cur_byte];
		val >>= (cur_bit & XOR_32(31));
		cur_bit += bits;

		auto ret = val;
		if ((cur_bit - 1) >> XOR_32(5) == cur_byte)
		{
			if (bits != XOR_32(32))
				ret &= bit_extra_masks[bits];
		}
		else
		{
			const auto extra = cur_bit & XOR_32(31);
			auto val2 = ((uint32_t *)data)[cur_byte + 1];
			val2 &= bit_extra_masks[bits];
			ret |= (val2 << (bits - extra));
		}

		return ret;
	}

	__forceinline float read_coord()
	{
		int int_val{}, fract_val{}, sign_bit{};
		float val{};

		int_val = read_bit();
		fract_val = read_bit();

		if (int_val || fract_val)
		{
			sign_bit = read_bit();
			if (int_val)
				int_val = (int)read_ulong(coord_integer_bits) + 1;
			if (fract_val)
				fract_val = (int)read_ulong(coord_fractional_bits);

			val = (float)int_val + ((float)fract_val * (float)coord_resolution);
			if (sign_bit)
				val = -val;
		}

		return val;
	}

	__forceinline uint8_t read_byte() { return read_ulong(sizeof(uint8_t) << XOR_32(3)) & XOR_32(0xFF); }
	__forceinline uint16_t read_word() { return read_ulong(sizeof(uint16_t) << XOR_32(3)) & XOR_32(0xFFFF); }
	__forceinline uint32_t read_dword() { return read_ulong(sizeof(uint32_t) << XOR_32(3)); }

private:
	char *data;
	size_t data_size;
	size_t data_bits;
	size_t cur_bit;
};
} // namespace sdk

#endif // SDK_BITBUF_H
