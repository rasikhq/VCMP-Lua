# Stream

A `Stream` is a buffer of bytes exchanged with client scripts. Write values
into one and `send` it to a player; data a client script sends arrives as a
`Stream` in `onClientData`. A stream is a plain value, not an entity.

## Creating

```
local stream = Stream()
local stream = Stream.new()
```

Both return an empty stream.

## Static functions

Only `Stream.new()`.

## Methods

Writes append at the end. Reads start at the beginning and advance a read
position of their own, so a stream can be written and then read back. A
read or write that fails changes nothing.

### stream:writeByte(value)

Writes one byte. `value` is an integer from -128 to 255; the low 8 bits
are written.

### stream:writeNumber(value)

Writes a 32-bit integer. `value` is an integer from -2147483648 to
4294967295, signed or unsigned.

### stream:writeFloat(value)

Writes `value` as a 32-bit float.

### stream:writeString(text)

Writes a string (a number is converted). `text` is at most 65535 bytes.

### stream:readByte()

Returns the next byte, 0 to 255.

### stream:readNumber()

Returns the next 32-bit integer, signed: a value written as `0xFFFFFFFF`
reads back as -1.

### stream:readFloat()

Returns the next 32-bit float. Precision is lost: a value written as `0.1`
reads back as `0.10000000149011612`, which is not `== 0.1`.

### stream:readString()

Returns the next string.

### stream:send([player])

Sends the bytes to `player`'s client scripts. Returns `true`, or `false` if
the server refuses. Without a player, or with `nil`, it sends to every
connected player and returns `true`.

### stream:clear()

Empties the stream and resets the read position, ready to write again.

### tostring(stream)

Returns `"Stream(<size> bytes)"`.

## Properties

| Name | Type | Access | Notes |
|---|---|---|---|
| `size` | integer | read | Bytes in the stream. |
| `remaining` | integer | read | Bytes not read yet. |

## Byte layout

The layout matches v1 and the client's streams:

| Value | Bytes |
|---|---|
| byte | 1 byte. |
| number | 4 bytes, little-endian, two's complement. |
| float | 4 bytes, little-endian, IEEE 754 single precision. |
| string | A 2-byte big-endian length, then the bytes. No terminator. |

For example, `writeNumber(0x01020304)` then `writeString("ab")` gives the
bytes `04 03 02 01 00 02 61 62`.

## Limits and errors

- A stream you write holds at most 4096 bytes. A write that does not fit
  raises `Stream: no room to write a byte (4096 of 4096 bytes used)` (or
  `a number`, `a float`, `a string`).
- A longer string raises `Stream: a string is at most 65535 bytes`.
- A read past the end raises `Stream: not enough data to read a number (3
  of 4 bytes left)` (or `a byte`, `a float`, `a string length`), or
  `Stream: not enough data to read a string of 10 bytes`.
- Bad arguments raise the usual errors, for example
  `bad argument #1 to 'writeByte' (value 256 out of range [-128, 255])`.
  See [entities](entities.md#argument-conventions).

## Receiving: onClientData

```
Event.bind("onClientData", function(player, stream, size) ... end)
```

- `stream` holds exactly the bytes the client sent; `size` is their
  number. Data of any size is accepted, also more than 4096 bytes; a
  received stream larger than 4096 bytes takes no writes.
- Every handler of one event gets the same `Stream`. A handler that reads
  moves the read position for the handlers after it.
- A read error in a handler is logged; the server carries on.

## Example

```lua
-- The client script sends an id (number) and a text (string);
-- the server answers with the text's length and a float.
Event.bind("onClientData", function(player, stream, size)
    local id = stream:readNumber()
    local text = stream:readString()

    local reply = Stream()
    reply:writeNumber(id)
    reply:writeNumber(#text)
    reply:writeFloat(player.health)
    reply:send(player)
end)
```
