#include "Stream.h"
#include "utility.h"

extern PluginFuncs* g_Funcs;

Stream::Stream()
	: m_ReadCursor(0), m_WriteCursor(0)
{}

Stream::Stream(const uint8_t* data, size_t size)
	: m_ReadCursor(0), m_WriteCursor(size)
{
	std::memcpy(m_Data, data, size);
	spdlog::debug("Stream instantiated with size: {}", size);
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
	spdlog::debug("Stream :: ReadByte :: [{} / {}] {}", m_ReadCursor, m_WriteCursor, dataSize);

	if (m_ReadCursor >= m_WriteCursor) {
		spdlog::error("Stream :: Not enough 'bytes' to read!");
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
		spdlog::error("Stream :: Not enough 'number' to read!");
		return 0;
	}

	size_t dataSize = sizeof(int);
	spdlog::debug("Stream :: ReadNumber :: [{} / {}] {}", m_ReadCursor, m_WriteCursor, dataSize);

	int data;
	std::memcpy(&data, &m_Data[m_ReadCursor], dataSize);
	m_ReadCursor += dataSize;
	
	return data;
}

float Stream::ReadFloat()
{
	if (m_ReadCursor >= m_WriteCursor) {
		spdlog::error("Stream :: Not enough 'float' to read!");
		return 0.0f;
	}

	size_t dataSize = sizeof(float);
	spdlog::debug("Stream :: ReadFloat :: [{} / {}] {}", m_ReadCursor, m_WriteCursor, dataSize);

	float data;
	std::memcpy(&data, &m_Data[m_ReadCursor], dataSize);
	m_ReadCursor += dataSize;

	return data;
}

std::string Stream::ReadString()
{
	spdlog::debug("Stream :: ReadString :: {} / {}", m_ReadCursor, m_WriteCursor);

	if (m_ReadCursor >= m_WriteCursor) {
		spdlog::error("Stream :: Not enough 'String' to read!");
		return std::string();
	}

	size_t dataSize = sizeof(uint16_t);
	uint16_t length;
	
	std::memcpy(&length, &m_Data[m_ReadCursor], dataSize);
	m_ReadCursor += dataSize;

	length = static_cast<uint16_t>(((length >> 8) & 0xFF) | ((length & 0xFF) << 8));

	spdlog::debug("m_ReadCursor: {0}\nm_WriteCursor: {1}\ndataSize: {2}\nlength: {3}", m_ReadCursor, m_WriteCursor, dataSize, length);

	if (m_ReadCursor + length > m_WriteCursor) {
		spdlog::error("Stream :: 'String' buffer exceeds out of capacity!");
		return std::string();
	}

	std::string result(&m_Data[m_ReadCursor], &m_Data[m_ReadCursor + length]);
	
	m_ReadCursor += length;
	return result;
}

/*******************************************************************************/

void Stream::WriteByte(uint8_t data)
{
	if (!isSpaceAvailable(sizeof(data))) {
		spdlog::error("Stream :: Stream max capacity of {} reached!", MAX_STREAM_DATA);
		return;
	}
	Write(&data, sizeof(data));
}

void Stream::WriteNumber(int data)
{
	if (!isSpaceAvailable(sizeof(data))) {
		spdlog::error("Stream :: Stream max capacity of {} reached!", MAX_STREAM_DATA);
		return;
	}
	Write(&data, sizeof(data));
}

void Stream::WriteFloat(float data)
{
	if (!isSpaceAvailable(sizeof(data))) {
		spdlog::error("Stream :: Stream max capacity of {} reached!", MAX_STREAM_DATA);
		return;
	}
	Write(&data, sizeof(data));
}

void Stream::WriteString(std::string data)
{
	if (data.length() > 0xFFFF) {
		spdlog::error("Stream :: String too large!");
		return;
	}
	uint16_t length = uint16_t(data.length());
	if (!isSpaceAvailable(sizeof(length)) || !isSpaceAvailable(length)) {
		spdlog::error("Stream :: Stream max capacity of {} reached!", MAX_STREAM_DATA);
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

std::ostream& operator<<(std::ostream& os, const Stream& e) { os << "StreamBuffer"; return os; }