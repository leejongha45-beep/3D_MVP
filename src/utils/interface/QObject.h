#pragma once

class QObject
{
public:
	virtual ~QObject() = default;

	virtual void initialize() = 0;
	virtual void update(float deltaSeconds) = 0;
	virtual void release() = 0;
};
