#pragma once

class QRenderObject
{
public:
	virtual ~QRenderObject() = default;

	virtual void create() = 0;
};
