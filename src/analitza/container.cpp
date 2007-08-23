/*************************************************************************************
 *  Copyright (C) 2007 by Aleix Pol <aleixpol@gmail.com>                             *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/
#include "container.h"

#include "expression.h"
#include "value.h"

#include <kdebug.h>

Container::Container(const Container& c) : Object(Object::container)
{
	Q_ASSERT(c.type()==Object::container);
	if(c.type()!=Object::container) {
		setType(Object::none);
		return;
	}
	
	m_params = c.copyParams();
}

Container::Container(const Object *o) : Object(o->type())
{
	Q_ASSERT(o!=0);
	if(type() == Object::container) {
		Container *c = (Container*) o;
		m_cont_type = c->containerType();
		
		m_params = c->copyParams();
	} else {
		setType(Object::none);
		m_cont_type = none;
	}
}

Operator Container::firstOperator() const
{
	Operator ret(Object::onone);
	bool found=false;
	for(int i=0; i<m_params.count() && !found; i++) {
		if(m_params[i]->type()==Object::oper) {
			ret = m_params[i];
			found = true;
		} else if(i==0 && containerType()==apply && m_params[i]->type()==Object::variable) {
			ret = function;
			found=true;
		}
	}
	
	return ret;
}

QString Container::toMathML() const
{
	QString ret;
	QList<Object*>::const_iterator i;
	for(i=m_params.constBegin(); i!=m_params.constEnd(); ++i) {
		if(*i==0)
			ret += "error;";
		else
			ret += (*i)->toMathML();
	}
	
	return QString("<%1>%2</%1>").arg(tagName()).arg(ret);
}

QString Container::toString() const
{
	QStringList ret;
	bool func=false;
	
	Operator *op=0;
	for(int i=0; i<m_params.count(); i++) {
		Q_ASSERT(m_params[i]!=0);
		
		if(m_params[i]->type() == Object::oper)
			op = (Operator*) m_params[i];
		else if(m_params[i]->type() == Object::variable) {
			Ci *b = (Ci*) m_params[i];
			func=b->isFunction();
			ret << b->toString();
		} else if(m_params[i]->type() == Object::container) {
			Container *c = (Container*) m_params[i];
			QString s = c->toString();
			Operator child_op = c->firstOperator();
			if(op!=0 && op->weight()>child_op.weight() && op->nparams()!=1) //apply
				s=QString("(%1)").arg(s);
			else if(c->containerType() == Container::bvar) { //bvar
				Container *ul = ulimit(), *dl = dlimit();
				if(ul!=0 || dl!=0) {
					if(dl!=0)
						s += dl->toString();
					s += "..";
					if(ul!=0)
						s += ul->toString();
				}
			}
			ret << s;
		} else 
			ret << m_params[i]->toString();
	}
	
	QString toret;
	switch(containerType()) {
		case declare:
			toret += ret.join(":=");
			break;
		case lambda:
			toret += ret.join("");
			break;
		case math:
			toret += ret.join("; ");
			break;
		case apply:
			if(func){
				QString n = ret.takeFirst();
				toret += QString("%1(%2)").arg(n).arg(ret.join(", "));
			} else if(op==0)
				toret += ret.join(" ");
			else switch(op->operatorType()) {
				case Object::plus:
					toret += ret.join("+");
					break;
				case Object::times:
					toret += ret.join("*");
					break;
				case Object::divide:
					toret += ret.join("/");
					break;
				case Object::minus:
					if(ret.count()==1)
						toret += '-'+ret[0];
					else
						toret += ret.join("-");
					break;
				case Object::power:
					toret += ret.join("^");
					break;
				default:
					toret += QString("%1(%2)").arg(op->toString()).arg(ret.join(", "));
					break;
			}
			break;
		case uplimit: //x->(n1..n2) is put at the same time
		case downlimit:
			break;
		case bvar:
			toret += ret.join("->")+"->";
			break;
		case piece:
			toret += ret[1]+" ? "+ret[0];
			break;
		case otherwise:
			toret += "? "+ret[0];
			break;
		default:
			toret += tagName()+'{'+ret.join(", ")+'}';
			break;
	}
	return toret;
}


QString Container::toHtml() const
{
	QStringList ret;
	bool func=false;
	
	Operator *op=0;
	for(int i=0; i<m_params.count(); i++) {
		Q_ASSERT(m_params[i]!=0);
		
		if(m_params[i]->type() == Object::oper)
			op = (Operator*) m_params[i];
		else if(m_params[i]->type() == Object::variable) {
			Ci *b = (Ci*) m_params[i];
			func=b->isFunction();
			ret << b->toHtml();
		} else if(m_params[i]->type() == Object::container) {
			Container *c = (Container*) m_params[i];
			QString s = c->toHtml();
			Operator child_op = c->firstOperator();
			if(op!=0 && op->weight()>child_op.weight() && op->nparams()!=1)
				s=QString("<span class='op'>(</span>%1<span class='op'>)</span>").arg(s);
			
			if(c->containerType() == Container::bvar) {
				Container *ul = ulimit(), *dl = dlimit();
				if(ul!=0 || dl!=0) {
					if(dl!=0)
						s += dl->toHtml();
					s += "<span class='op'>..</span>";
					if(ul!=0)
						s += ul->toHtml();
				}
			}
			
			if(c->containerType()!=Container::uplimit && c->containerType()!=Container::downlimit)
				ret << s;
		} else 
			ret << m_params[i]->toHtml();
	}
	
	QString toret;
	switch(containerType()) {
		case declare:
			toret += ret.join("<span class='op'>:=</span>");
			break;
		case lambda:
			toret += ret.join("");
			break;
		case math:
			toret += ret.join("<span class='op'>;</span> ");
			break;
		case apply:
			if(func){
				QString n = ret.takeFirst();
				toret += QString("%1<span class='op'>(</span>%2<span class='op'>)</span>").arg(n).arg(ret.join(", "));
			} else if(op==0)
				toret += ret.join(" ");
				else switch(op->operatorType()) {
					case Object::plus:
						toret += ret.join("<span class='op'>+</span>");
						break;
					case Object::times:
						toret += ret.join("<span class='op'>*</span>");
						break;
					case Object::divide:
						toret += ret.join("<span class='op'>/</span>");
						break;
					case Object::minus:
						if(ret.count()==1)
							toret += "<span class='op'>-</span>"+ret[0];
						else
							toret += ret.join("<span class='op'>-</span>");
						break;
					case Object::power:
						toret += ret.join("<span class='op'>^</span>");
						break;
					default:
						toret += QString("%1<span class='op'>(</span>%2<span class='op'>)</span>").arg(op->toString()).arg(ret.join(", "));
						break;
				}
				break;
				case uplimit: //x->(n1..n2) is put at the same time
		case downlimit:
			break;
		case bvar:
			toret += ret.join("<span class='op'>-&gt;</span>")+"<span class='op'>-&gt;</span>";
			break;
		case piece:
			toret += ret[1]+" <span class='op'>?</span> "+ret[0];
			break;
		case otherwise:
			toret += "<span class='op'>?</span> "+ret[0];
			break;
		default:
			toret += tagName()+"<span class='op'>{</span>"+ret.join("<span class='op'>,</span> ")+"<span class='op'>}</span>";
			break;
	}
	return toret;
}

QList<Object*> Container::copyParams() const
{
	QList<Object*> ret;
	
	for(QList<Object*>::const_iterator it=m_params.begin(); it!=m_params.end(); it++) {
		ret.append(Expression::objectCopy(*it));
	}
	return ret;
}

enum Container::ContainerType Container::toContainerType(const QString& tag)
{
	ContainerType ret=none;
	
	if(tag=="apply") ret=apply;
	else if(tag=="declare") ret=declare;
	else if(tag=="math") ret=math;
	else if(tag=="lambda") ret=lambda;
	else if(tag=="bvar") ret=bvar;
	else if(tag=="uplimit") ret=uplimit;
	else if(tag=="downlimit") ret=downlimit;
	else if(tag=="piecewise") ret=piecewise;
	else if(tag=="piece") ret=piece;
	else if(tag=="otherwise") ret=otherwise;
	
	return ret;
}

QStringList Container::bvarList() const //NOTE: Should return Ci's instead of Strings?
{
	QStringList bvars;
	QList<Object*>::const_iterator it;
	
	for(it=m_params.begin(); it!=m_params.end(); ++it) {
		Container* c = (Container*) (*it);
		if(c->containerType() == Container::bvar)
			bvars.append(((Ci*)c->m_params[0])->name());
	}
	
	return bvars;
}

Container* Container::ulimit() const
{
	for(QList<Object*>::const_iterator it=m_params.begin(); it!=m_params.end(); ++it) {
		Container *c = (Container*) (*it);
		if(c->type()==Object::container && c->containerType()==Container::uplimit && c->m_params[0]->type()==Object::value)
			return (Container*) c->m_params[0];
	}
	return 0;
}

Container* Container::dlimit() const
{
	for(QList<Object*>::const_iterator it=m_params.begin(); it!=m_params.end(); ++it) {
		Container *c = (Container*) (*it);
		if(c->type()==Object::container && c->containerType()==Container::downlimit && c->m_params[0]->type()==Object::value)
			return (Container*) c->m_params[0];
	}
	return 0;
}

bool Container::hasVars() const
{
	bool ret=false;
	
	if(m_params.isEmpty())
		ret = false;
	else {
		for(QList<Object*>::const_iterator i=m_params.begin(); !ret && i!=m_params.end(); ++i) {
			switch((*i)->type()) {
				case Object::variable:
					ret=true;
					break;
				case Object::container:
				{
					Container *c = (Container*) *i;
					ret |= c->hasVars();
					break;
				}
				default:
					ret=false;
			}
		}
	}
	return ret;
}

bool Container::operator==(const Container& c) const
{
	bool eq=c.m_params.count()==m_params.count();
	QList<Object*>::const_iterator it = c.m_params.begin();
	
	for(int i=0; eq && i<m_params.count(); ++i) {
		Object *o=m_params[i], *o1=c.m_params[i];
		eq = eq && equalTree(o, o1);
	}
	return eq;
}

bool Container::equalTree(Object const* o1, Object const * o2)
{
	Q_ASSERT(o1 && o2);
	if(o1==o2)
		return true;
	bool eq= o1->type()==o2->type();
	switch(o2->type()) {
		case Object::variable:
			eq = eq && Ci(o2)==Ci(o1);
			break;
		case Object::value:
			eq = eq && Cn(o2)==Cn(o1);
			break;
		case Object::container:
			eq = eq && Container(o2)==Container(o1);
			break;
		case Object::oper:
			eq = eq && Operator(o2)==Operator(o1);
			break;
		default:
			break;
	}
	return eq;
}

#if 0
Container::Container(Cn* base, Object* var, Ci* degree) : Object(Object::container)
{
	if(!var)
		return;
	else if(!base && var && !degree) {
		m_params.append(new Operator(Object::times));
		m_params.append(new Cn(1.));
		m_params.append(var);
	} else if(base && var && !degree) {
		m_params.append(new Operator(Object::times));
		m_params.append(base);
		m_params.append(var);
	} else if(!base && var && degree) {
		m_params.append(new Operator(Object::power));
		m_params.append(var);
		m_params.append(degree);
	} else {
		m_params.append(new Operator(Object::times));
		m_params.append(base);
		m_params.append(var);
		m_params.append(new Container(0, var, degree));
	}
}

Cn* Container::monomialDegree(const Container& c)
{
	bool valid=false;
	int scalar=-1, var=-1;
	
	if(c.m_params[1]->type()==Object::value) {
		scalar=1;
		var=2;
		valid=true;
	} else if(c.m_params[2]->type()==Object::value) {
		scalar=2;
		var=1;
		valid=true;
	}
	
	if(c.firstOperator()==Object::power) {
		return (Cn*) c.m_params[scalar];
	} else if(c.firstOperator()==Object::times) {
		return monomialDegree(c.m_params[var]);
	}
	return 0;
}

Cn* Container::monomialBase(const Container& c)
{
	if(c.firstOperator()==Object::times) {
		bool valid=false;
		int scalar=-1, var=-1;
		
		if(c.m_params[1]->type()==Object::value) {
			scalar=1;
			var=2;
			valid=true;
		} else if(c.m_params[2]->type()==Object::value) {
			scalar=2;
			var=1;
			valid=true;
		}
		
		if(valid)
			return (Cn*) c.m_params[scalar];
	}
	return 0;
}

Object* Container::monomialVar(const Container& c) //FIXME: Must improve these vars
{
	bool valid=false;
	int scalar=-1, var=-1;
	if(c.m_params[1]->type()==Object::value) {
		scalar=1;
		var=2;
		valid=true;
	} else if(c.m_params[2]->type()==Object::value) {
		scalar=2;
		var=1;
		valid=true;
	}
	
	if(valid) {
		Object *o = monomialVar(c.m_params[var]);
		if(!o)
			return c.m_params[var];
		else
			return o;
	}
	
	return 0;
}
#endif

void objectWalker(const Object* root, int ind)
{
	Container *c; Cn *num; Operator *op; Ci *var;
	QString s;
	if(!root) {
		qDebug() << "This is an null object";
		return;
	}
	
	if(ind>100) return;
	
	for(int i=0; i<ind; i++)
		s += " |_____";
	
	switch(root->type()) { //TODO: include the function into a module and use toString
		case Object::container:
			c= (Container*) root;
			qDebug() << qPrintable(s) << "| cont: " << c->tagName() << "=" << c->toString();
			for(int i=0; i<c->m_params.count(); i++)
				objectWalker(c->m_params[i], ind+1);
			
			break;
		case Object::value:
			num= (Cn*) root;
			qDebug() << qPrintable(s) << "| num: " << num->value();
			break;
		case Object::oper:
			op= (Operator*) root;
			qDebug() << qPrintable(s) << "| operator: " << op->toString();
			break;
		case Object::variable:
			var = (Ci*) root;
			qDebug() << qPrintable(s) << "| variable: " << var->name() << "Func:" << var->isFunction();
			break;
		default:
			qDebug() << qPrintable(s) << "| dunno: " << (int) root->type();
			break;
	}
}

void print_dom(const QDomNode& in, int ind)
{
	QString a;

	if(ind >100){
		qDebug("...");
		return;
	}
	
	for(int i=0; i<ind; i++)
		a.append("______|");

	if(in.hasChildNodes())
		qDebug("%s%s(%s) -- %d", qPrintable(a), qPrintable(in.toElement().tagName()), qPrintable(in.toElement().text()), in.childNodes().length());
	else
		qDebug("%s%s", qPrintable(a), qPrintable(in.toElement().tagName()));

	for(unsigned int i=0 ; i<in.childNodes().length(); i++){
		if(in.childNodes().item(i).isElement())
			print_dom(in.childNodes().item(i), ind+1);
	}
}


bool Container::isNumber() const
{
	return m_cont_type==apply || m_cont_type==math || m_cont_type==lambda ||
		m_cont_type==piecewise || m_cont_type==piece || m_cont_type==otherwise;
}

QList<Object *>::iterator Container::firstValue()
{
	QList<Object *>::iterator it(m_params.begin()), itEnd(m_params.end());
	for(; it!=itEnd; ++it) {
		bool found=false;
		
		switch((*it)->type()) {
			case Object::value:
			case Object::variable:
				found=true;
				break;
			case Object::container:
				if(((Container*) *it)->isNumber())
					found=true;
				break;
			default:
				break;
		}
		if(found)
			break;
	}
	return it;
}

QList<Object *>::const_iterator Container::firstValue() const
{
	QList<Object *>::const_iterator it(m_params.constBegin()), itEnd(m_params.constEnd());
	for(; it!=itEnd; ++it) {
		bool found=false;
		
		switch((*it)->type()) {
			case Object::value:
			case Object::variable:
				found=true;
				break;
			case Object::container:
				if(((Container*) *it)->isNumber())
					found=true;
				break;
			default:
				break;
		}
		if(found)
			break;
	}
	return it;
}

bool Container::isUnary() const
{
	QList<Object*>::const_iterator it(firstValue());
	return ++it==m_params.end();
}

bool Container::isCorrect() const
{
	return m_correct && m_type==Object::container && m_cont_type!=none/* && !isEmpty()*/;
}

QString Container::tagName() const
{
	QString tag;
	switch(m_cont_type) {
		case declare:
			tag="declare";
			break;
		case lambda:
			tag="lambda";
			break;
		case math:
			tag="math";
			break;
		case apply:
			tag="apply";
			break;
		case uplimit:
			tag="uplimit";
			break;
		case downlimit:
			tag="downlimit";
			break;
		case bvar:
			tag="bvar";
			break;
		case piece:
			tag="piece";
			break;
		case piecewise:
			tag="piecewise";
			break;
		case otherwise:
			tag="otherwise";
			break;
		case none:
			break;
	}
	return tag;
}


