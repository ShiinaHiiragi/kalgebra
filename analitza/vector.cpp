/*
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "vector.h"
#include "expression.h"
#include "expressionwriter.h"

Vector::Vector(const Vector& v)
	: Object(Object::vector)//, m_elements(v.m_elements.size())
{
	foreach(const Object* o, v.m_elements)
	{
		m_elements.append(Expression::objectCopy(o));
	}
}

Vector::~Vector()
{
	qDeleteAll(m_elements);
}

Vector::Vector(int size)
	: Object(Object::vector)//, m_elements(size)
{}

Vector::Vector(const Object* o)
	: Object(Object::vector)
{
	Q_ASSERT(o->type()==Object::vector);
	const Vector *v=static_cast<const Vector*>(o);
// 	m_elements.reserve(v->m_elements.size());
	foreach(const Object* o, v->m_elements)
	{
		m_elements.append(Expression::objectCopy(o));
	}
}

void Vector::appendBranch(Object* o)
{
	m_elements.append(o);
}

QString Vector::visit(ExpressionWriter* e) const
{
	return e->accept(this);
}

bool Vector::isCorrect() const
{
	bool corr = !m_elements.isEmpty();
	foreach(const Object* o, m_elements) {
		corr |= o->isCorrect();
	}
	return corr;
}

void Vector::negate()
{
	foreach(Object* o, m_elements) {
		o->isCorrect();
	}
}

bool Vector::isZero() const
{
	foreach(const Object* o, m_elements) {
		if(!o->isZero())
			return false;
	}
	return true;
}

