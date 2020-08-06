#include "Stream.h"
#include "utility.h"

#define DEBUG_STREAM
#undef DEBUG_STREAM

extern PluginFuncs* g_Funcs;

Stream::Stream()
	: m_ReadCursor(0), m_WriteCursor(0)
{}

Stream::Stream(const uint8_t* data, size_t size)
	: m_ReadCursor(0), m_WriteCursor(0)
{
	std::memcpy(m_Data, data, size);
	m_WriteCursor += size;
#ifdef DEBUG_STREAM
	std::cout << "Stream instantiated with size: " << size << std::endl;
#endif
}

Stream::~Stream()
{
	m_WriteCursor = 0;
	m_ReadCursor = 0;
}

/*******************************************************************************/

uint8_t Stream::ReadByte()
{
	size_t dataSize = sizeof(uint8_t);
#ifdef DEBUG_STREAM
	std::cout << "Stream :: ReadByte :: [" << m_ReadCursor << "] " << dataSize << std::endl;
#endif

	if (m_ReadCursor >= m_WriteCursor) {
		OutputError("Stream :: Not enough 'bytes' to read!");
		return 0x0;
	}

	uint8_t data;
	std::memcpy(&data, &m_Data[m_ReadCursor], dataSize);
	m_ReadCursor += dataSize;

	return data;
}

int Stream::ReadNumber()
{
	if (m_ReadCursor >= m_WriteCursor) {
		OutputError("Stream :: Not enough 'number' to read!");
		return 0;
	}

	size_t dataSize = sizeof(int);
#ifdef DEBUG_STREAM
	std::cout << "Stream :: ReadNumber :: [" << m_ReadCursor << "] " << dataSize << std::endl;
#endif

	int data;
	std::memcpy(&data, &m_Data[m_ReadCursor], dataSize);
	m_ReadCursor += dataSize;
	
	return data;
}

float Stream::ReadFloat()
{
	if (m_ReadCursor >= m_WriteCursor) {
		OutputError("Stream :: Not enough 'float' to read!");
		return 0.0f;
	}

	size_t dataSize = sizeof(float);
#ifdef DEBUG_STREAM
	std::cout << "Stream :: ReadFloat :: [" << m_ReadCursor << "] " << dataSize << std::endl;
#endif

	float data;
	std::memcpy(&data, &m_Data[m_ReadCursor], dataSize);
	m_ReadCursor += dataSize;

	return data;
}

std::string Stream::ReadString()
{
#ifdef DEBUG_STREAM
	std::cout << "Stream :: ReadString :: [" << m_ReadCursor << "] " << std::endl;
#endif

	if (m_ReadCursor >= m_WriteCursor) {
		OutputError("Stream :: Not enough 'String' to read!");
		return std::string();
	}

	size_t dataSize = sizeof(uint16_t);
	uint16_t length;
	
	std::memcpy(&length, &m_Data[m_ReadCursor], dataSize);
	m_ReadCursor += dataSize;

	length = ((length >> 8) & 0xFF) | ((length & 0xFF) << 8);

#ifdef DEBUG_STREAM
	std::cout << "m_ReadCursor: " << m_ReadCursor << "\n"
		<< "m_WriteCursor: " << m_WriteCursor << "\n"
		<< "dataSize: " << dataSize << "\n"
		<< "length: " << length << std::endl;
#endif

	if (m_ReadCursor + length > m_WriteCursor) {
		OutputError("Stream :: 'String' buffer exceeds out of capacity!");
		return std::string();
	}

	static char data[MAX_STREAM_DATA];
	std:memcpy(&data, &m_Data[m_ReadCursor], length);
	
	m_ReadCursor += length;
	return std::string(data);
}

/*******************************************************************************/

void Stream::WriteByte(uint8_t data)
{
	if (!isSpaceAvailable(sizeof(data))) {
		OutputError("Stream :: Stream max capacity of %d reached!", MAX_STREAM_DATA);
		return;
	}
	Write(&data, sizeof(data));
}

void Stream::WriteNumber(int data)
{
	if (!isSpaceAvailable(sizeof(data))) {
		OutputError("Stream :: Stream max capacity of %d reached!", MAX_STREAM_DATA);
		return;
	}
	Write(&data, sizeof(data));
}

void Stream::WriteFloat(float data)
{
	if (!isSpaceAvailable(sizeof(data))) {
		OutputError("Stream :: Stream max capacity of %d reached!", MAX_STREAM_DATA);
		return;
	}
	Write(&data, sizeof(data));
}

void Stream::WriteString(std::string data)
{
	if (data.length() > 0xFFFF) {
		OutputError("Stream :: String too large!");
		return;
	}
	uint16_t length = uint16_t(data.length());
	if (!isSpaceAvailable(sizeof(length)) || !isSpaceAvailable(length)) {
		OutputError("Stream :: Stream max capacity of %d reached!", MAX_STREAM_DATA);
		return;
	}
	uint16_t size = ((length >> 8) & 0xFF) | ((length & 0xFF) << 8);
	Write(&size, sizeof(size));
	Write(data.c_str(), length);
}

bool Stream::Write(const void* data, size_t size) {
	std::memcpy(&m_Data[m_WriteCursor], data, size);
	m_WriteCursor += size;
	return true;
}


/*******************************************************************************/

void Stream::Send(Player* player) {
	if (!player) {
		for (const auto& p : Player::s_Players) {
			g_Funcs->SendClientScriptData(p.getID(), m_Data, m_WriteCursor);
		}
	}
	else {
		g_Funcs->SendClientScriptData(player->getID(), m_Data, m_WriteCursor);
	}
}

void Stream::Init(sol::state* L) {
	sol::usertype<Stream> userdata = L->new_usertype<Stream>("Stream",
		sol::constructors<
			Stream()
		>()
	);

	userdata["readByte"] = &Stream::ReadByte;
	userdata["readNumber"] = &Stream::ReadNumber;
	userdata["readFloat"] = &Stream::ReadFloat;
	userdata["readString"] = &Stream::ReadString;
	
	userdata["writeByte"] = &Stream::WriteByte;
	userdata["writeNumber"] = &Stream::WriteNumber;
	userdata["writeFloat"] = &Stream::WriteFloat;
	userdata["writeString"] = &Stream::WriteString;

	userdata["send"] = &Stream::Send;
	userdata["clear"] = &Stream::Clear;
}