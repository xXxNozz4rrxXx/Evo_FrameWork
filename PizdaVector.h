#pragma once
template <typename T>
class PizdaVector
{
public:
	int                         Size;
	int                         Capacity;
	T*                          Data;

	typedef T                   value_type;
	typedef value_type*         iterator;
	typedef const value_type*   const_iterator;

	inline PizdaVector() { Size = Capacity = 0; Data = NULL; }
	inline ~PizdaVector() { if (Data) free(Data);}
	inline PizdaVector(const PizdaVector<T>& src) { Size = Capacity = 0; Data = NULL; operator=(src); }
	inline PizdaVector& operator=(const PizdaVector<T>& src) { clear(); resize(src.Size); memcpy(Data, src.Data, (size_t)Size * sizeof(value_type)); return *this; }

	inline bool                 empty() const { return Size == 0; }
	inline int                  size() const { return Size; }
	inline int                  capacity() const { return Capacity; }
	inline value_type&          operator[](int i) { return Data[i]; }
	inline const value_type&    operator[](int i) const { return Data[i]; }

	inline void                 clear() { if (Data) { Size = Capacity = 0; free(Data); Data = NULL; } }
	inline iterator             begin() { return Data; }
	inline const_iterator       begin() const { return Data; }
	inline iterator             end() { return Data + Size; }
	inline const_iterator       end() const { return Data + Size; }
	inline value_type&          front() { return Data[0]; }
	inline const value_type&    front() const { return Data[0]; }
	inline value_type&          back() {return Data[Size - 1]; }
	inline const value_type&    back() const { return Data[Size - 1]; }
	inline void                 swap(PizdaVector<value_type>& rhs) { int rhs_size = rhs.Size; rhs.Size = Size; Size = rhs_size; int rhs_cap = rhs.Capacity; rhs.Capacity = Capacity; Capacity = rhs_cap; value_type* rhs_data = rhs.Data; rhs.Data = Data; Data = rhs_data; }

	inline int          _grow_capacity(int sz) const { int new_capacity = Capacity ? (Capacity + Capacity / 2) : 8; return new_capacity > sz ? new_capacity : sz; }
	inline void         resize(int new_size) { if (new_size > Capacity) reserve(_grow_capacity(new_size)); Size = new_size; }
	inline void         resize(int new_size, const value_type& v) { if (new_size > Capacity) reserve(_grow_capacity(new_size)); if (new_size > Size) for (int n = Size; n < new_size; n++) memcpy(&Data[n], &v, sizeof(v)); Size = new_size; }
	inline void         reserve(int new_capacity)
	{
		if (new_capacity <= Capacity)
			return;
		value_type* new_data = (value_type*)malloc((size_t)new_capacity * sizeof(value_type));
		if (Data)
			memcpy(new_data, Data, (size_t)Size * sizeof(value_type));
		free(Data);
		Data = new_data;
		Capacity = new_capacity;
	}
	 
	// NB: &v cannot be pointing inside the ImVector Data itself! e.g. v.push_back(v[10]) is forbidden.
	inline void         push_back(const value_type& v) { if (Size == Capacity) reserve(_grow_capacity(Size + 1)); memcpy(&Data[Size], &v, sizeof(v)); Size++; }
	inline void         pop_back() {Size--; }
	inline void         push_front(const value_type& v) { if (Size == 0) push_back(v); else insert(Data, v); }
	inline iterator     erase(const_iterator it) {const ptrdiff_t off = it - Data; memmove(Data + off, Data + off + 1, ((size_t)Size - (size_t)off - 1) * sizeof(value_type)); Size--; return Data + off; }
	inline iterator     insert(const_iterator it, const value_type& v) { const ptrdiff_t off = it - Data; if (Size == Capacity) reserve(_grow_capacity(Size + 1)); if (off < (int)Size) memmove(Data + off + 1, Data + off, ((size_t)Size - (size_t)off) * sizeof(value_type)); memcpy(&Data[off], &v, sizeof(v)); Size++; return Data + off; }
	inline bool         contains(const value_type& v) const { const T* data = Data;  const T* data_end = Data + Size; while (data < data_end) if (*data++ == v) return true; return false; }
};

