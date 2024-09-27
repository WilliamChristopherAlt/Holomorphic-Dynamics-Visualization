#pragma once

#include<vector>
#include <stdexcept>

int searchSorted(const std::vector<float>& vec, float value) {
	if (value < vec.front() || value > vec.back())
		return -1;
	auto it = std::lower_bound(vec.begin(), vec.end(), value);
	return it - vec.begin();
}

template <typename T>
class Spline
{
protected:
    std::vector<float> positions;
    std::vector<T> controlPoints;
    int size;

public:
    Spline(const std::vector<float>& positions_, const std::vector<T>& controlPoints_)
        : positions(positions_), controlPoints(controlPoints_), size(positions.size())
    {
        if (positions.size() != controlPoints.size())
            throw std::invalid_argument("Size of positions and controlPoints must be the same");
    }

    virtual T interpolate(float interVal) const = 0;

    virtual ~Spline() = default;
};

template <typename T>
class MonotonicHermite : public Spline<T>
{
public:
    MonotonicHermite(const std::vector<float>& positions_, const std::vector<T>& controlPoints_)
        : Spline<T>(positions_, controlPoints_) {}

    T interpolate(float interVal) const override
    {
        int i;
        if (interVal <= this->positions.front())
            i = 0;
        else if (interVal >= this->positions.back())
            i = this->size - 2;
        else
            i = searchSorted(this->positions, interVal) - 1;

        T p0 = this->controlPoints[i];
        T p1 = this->controlPoints[i + 1];
        T v0 = T(0.0f);
        T v1 = T(0.0f);

        T q0 = p0;
        T q1 = v0;
        T q2 = -3.0f * p0 - 2.0f * v0 + 3.0f * p1 - v1;
        T q3 = 2.0f * p0 + v0 - 2.0f * p1 + v1;

        float t = (interVal - this->positions[i]) / (this->positions[i + 1] - this->positions[i]);
        return q0 + q1 * t + q2 * t * t + q3 * t * t * t;
    }
};

template <typename T>
class LinearCyclic : public Spline<T>
{
public:
    LinearCyclic(const std::vector<float>& positions_, const std::vector<T>& controlPoints_)
        : Spline<T>(positions_, controlPoints_) {}

    T interpolate(float interVal) const override
    {
        int i;
        if (interVal <= this->positions.front())
            i = 0;
        else if (interVal >= this->positions.back())
            i = this->size - 1;
        else
            i = searchSorted(this->positions, interVal) - 1;

        T p0;
        T p1;

        float t;
        if (i == this->size - 1)
        {
            t = (interVal - this->positions[i]) / (1.0f - this->positions[i]);
            p0 = this->controlPoints[i];
            p1 = this->controlPoints.front();
        }
        else
        {
            t = (interVal - this->positions[i]) / (this->positions[i + 1] - this->positions[i]);
            p0 = this->controlPoints[i];
            p1 = this->controlPoints[i + 1];
        }
        return p0 + (p1 - p0) * t;
    }
};

template <typename T>
class MonotonicHermiteCyclic : public Spline<T>
{
public:
    MonotonicHermiteCyclic(const std::vector<float>& positions_, const std::vector<T>& controlPoints_)
        : Spline<T>(positions_, controlPoints_) {}

    T interpolate(float interVal) const override
    {
        int i;
        if (interVal <= this->positions.front())
            i = 0;
        else if (interVal >= this->positions.back())
            i = this->size - 1;
        else
            i = searchSorted(this->positions, interVal) - 1;

        T p0 = this->controlPoints[i];
        T p1 = (i == this->size - 1) ? this->controlPoints.front() : this->controlPoints[i + 1];
        T v0 = T(0.0f);
        T v1 = T(0.0f);

        T q0 = p0;
        T q1 = v0;
        T q2 = -3.0f * p0 - 2.0f * v0 + 3.0f * p1 - v1;
        T q3 = 2.0f * p0 + v0 - 2.0f * p1 + v1;

        float t = (interVal - this->positions[i]) / ((i == this->size - 1) ? (1.0f - this->positions[i]) : (this->positions[i + 1] - this->positions[i]));
        return q0 + q1 * t + q2 * t * t + q3 * t * t * t;
    }
};