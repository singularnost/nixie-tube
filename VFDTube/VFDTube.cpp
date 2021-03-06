/*
 * VFDTube.cpp
 *
 *  Created on: Oct 29, 2012
 *      Author: agu
 */

#include "VFDTube.h"

VFDTube::VFDTube(uint8_t pin_din, uint8_t pin_oe, uint8_t pin_st,
		uint8_t pin_sh, byte section_count) :
		_pin_din(pin_din), _pin_oe(pin_oe), _pin_st(pin_st), _pin_sh(pin_sh), _section_count(
				section_count) {
	_buff = (byte *) malloc(sizeof(byte) * (section_count) * _BYTE_PER_SECTION);

	pinMode(_pin_din, OUTPUT);
	pinMode(_pin_st, OUTPUT);
	pinMode(_pin_sh, OUTPUT);
	pinMode(_pin_oe, OUTPUT);

	this->setBrightness();
}

VFDTube::~VFDTube() {
	free(_buff);
}

void VFDTube::send(byte data) const {
	for (byte i = 8; i--; data <<= 1) {
		digitalWrite(_pin_din, data & 0x80);
		digitalWrite(_pin_sh, HIGH);
		digitalWrite(_pin_sh, LOW);
	}
}

void VFDTube::display() {
	for (byte i = 0; i < _section_count; i++) {
		this->send(_buff[i * _BYTE_PER_SECTION]);
		this->send(_buff[i * _BYTE_PER_SECTION + 1]);
	}

	digitalWrite(_pin_st, HIGH);
	digitalWrite(_pin_st, LOW);
}

void VFDTube::display(word millis) {
	this->display();
	delay(millis);
}

void VFDTube::setSection(byte index, word value) {
	index %= _section_count;
	_buff[index + index] = highByte(value);
	_buff[index + index + 1] = lowByte(value);
}

void VFDTube::clear(word value) {
	for (byte i = 0; i < _section_count; i++)
		this->setSection(i, value);
}

void VFDTube::setBackgroundColor(Color color) {
	for (byte i = 0; i < _section_count; i++) {
		this->setBackgroundColor(i, color);
	}
}

void VFDTube::setBackgroundColor(byte index, Color color) {
	index %= _section_count;
	_buff[index * _BYTE_PER_SECTION] = color;
}

void VFDTube::setPoint(byte index) {
	index %= _section_count;
	_buff[index * _BYTE_PER_SECTION + 1] |= 0x40;
}

void VFDTube::setPattern(byte index, byte pattern) {
	_buff[index * _BYTE_PER_SECTION + 1] &= 0x40;
	_buff[index * _BYTE_PER_SECTION + 1] |= pattern;
}

byte VFDTube::getPattern(byte index) {
	return _buff[index * _BYTE_PER_SECTION + 1] & 0xbf;
}

bool VFDTube::setChar(byte index, char c) {
	index %= _section_count;

	bool val = displayable(c);

	if (val) {
		if (c >= '0' && c <= '9')
			this->setPattern(index, pgm_read_byte_near(VFDTUBE_FONT + c - '0'));
		else if (c >= 'A' && c <= 'Z')
			this->setPattern(index,
					pgm_read_byte_near(VFDTUBE_FONT + c - 'A' + 10));
		else if (c >= 'a' && c <= 'z')
			this->setPattern(index,
					pgm_read_byte_near(VFDTUBE_FONT + c - 'a' + 10));
	}

	return val;
}

void VFDTube::setChar(char c) {
	for (byte i = 0; i < _section_count; i++)
		this->setChar(i, c);
}

void VFDTube::printf(const char *__fmt, ...) {
	word cache_length = _section_count * 2 + 1;
	char * cache = (char *) malloc(sizeof(char) * cache_length);

	va_list ap;
	va_start(ap, __fmt);
	vsnprintf(cache, cache_length, __fmt, ap);
	va_end(ap);

	byte index = 0;
	byte ptr = 0;

	for (byte i = 0; i < _section_count; i++)
		_buff[i * _BYTE_PER_SECTION + 1] = 0x00;

	while (cache[index] && ptr < _section_count) {
		if (this->setChar(ptr, cache[index]))
			ptr++;
		else if (cache[index] == '.')
			this->setPoint(ptr ? ptr - 1 : 0);
		else {
			_buff[ptr * _BYTE_PER_SECTION + 1] = 0x00;
			ptr++;
		}

		index++;
	}

	if (cache[index] == '.')
		this->setPoint(ptr - 1);

	free(cache);
}

bool VFDTube::displayable(char c) {
	return ((c >= '0' && c <= '9') or (c >= 'A' && c <= 'Z')
			or (c >= 'a' && c <= 'z'));
}

void VFDTube::setBrightness(byte brightness) {
	if (digitalPinToTimer(_pin_oe) == NOT_ON_TIMER)
		digitalWrite(_pin_oe, brightness ? LOW : HIGH);
	else
		analogWrite(_pin_oe, 0xff - brightness);
}
