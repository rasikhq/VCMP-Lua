#include "pch.h"
#include "Player.h"

#define MAX_STREAM_DATA 4096

class Stream {
public:
	Stream();
	Stream(const uint8_t* data, size_t size);
	~Stream();
	inline void Clear() { delete[] m_Data; }

	static void Init(sol::state* L);

	inline bool isSpaceAvailable(size_t size) { return (m_WriteCursor + size) < MAX_STREAM_DATA; }

	uint8_t ReadByte();
	int ReadNumber();
	float ReadFloat();
	std::string ReadString();

	void WriteByte(uint8_t data);
	void WriteNumber(int data);
	void WriteFloat(float data);
	void WriteString(std::string data);
	bool Write(const void* data, size_t size);

	void Send(Player* player);
private:
	uint8_t* m_Data;
	size_t m_ReadCursor;
	size_t m_WriteCursor;
	//size_t m_EndPos;
};