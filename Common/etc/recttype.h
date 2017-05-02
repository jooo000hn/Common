//
// 2017-04-20, jjuiddong
// Rect Class
//
#pragma once


namespace common
{
	
	template <class T>
	struct sRect1
	{
		static sRect1 Rect(T x, T y, T width, T height) {
			return sRect1<T>(x, y, x + width, y + height);
		}

		sRect1() {
		}

		sRect1(T left, T top, T right, T bottom) {
			this->left = left;
			this->top = top;
			this->right = right;
			this->bottom = bottom;
		}

		bool IsIn(const T x, const T y) {
			return (left <= x)
				&& (right >= x)
				&& (top <= y)
				&& (bottom >= y);
		}
		void SetX(const T x) {
			*this = sRect1<T>(x, top, x + Width(), bottom);
		}
		void SetY(const T y) {
			*this = sRect1(left, y, right, y + Height());
		}	
		void SetWidth(const T width) {
			*this = sRect1(left, top, width, Height());
		}
		void SetHeight(const T height) {
			*this = sRect1(left, top, right, height);
		}
		T Width() const {
			return abs(right - left);
		}
		T Height() const {
			return abs(bottom - top);
		}

		sRect1 operator-(const sRect1 &rhs) {
			return sRect1(left - rhs.left, 
				top - rhs.top, 
				right - rhs.right, 
				bottom - rhs.bottom);
		}
		sRect1 operator+(const sRect1 &rhs) {
			return sRect1(left + rhs.left,
				top + rhs.top,
				right + rhs.right,
				bottom + rhs.bottom);
		}
		sRect1& operator=(const RECT &rhs) {
			left = (T)rhs.left;
			right = (T)rhs.right;
			top = (T)rhs.top;
			bottom = (T)rhs.bottom;
			return *this;
		}
		bool operator==(const sRect1 &rhs) {
			return (left == rhs.left)
				&& (right == rhs.right)
				&& (top == rhs.top)
				&& (bottom == rhs.bottom);
		}

		T left, top, right, bottom;
	};



	//---------------------------------------------------------------------
	typedef sRect1<LONG> sRecti;
	typedef sRect1<float> sRectf;
}