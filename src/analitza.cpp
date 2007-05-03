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

#include "analitza.h"
#include <klocale.h>

#include "value.h"
#include "variables.h"
#include "container.h"
Analitza::Analitza() : m_vars(new Variables) { }

Analitza::Analitza(const Analitza& a) : m_exp(a.m_exp), m_err(a.m_err)
{
	m_vars = new Variables(*a.m_vars);
}

Analitza::~Analitza()
{
	if(m_vars)
		delete m_vars;
}


void Analitza::setExpression(const Expression & e)
{
	m_exp=e;
}

Expression Analitza::evaluate()
{
	m_err=QStringList();
	if(m_exp.isCorrect()) {
		Expression e(m_exp); //FIXME: That's a strange trick, wouldn't have to copy
		Object *aux=e.m_tree;
		e.m_tree=eval(aux, true);
		delete aux;
// 		objectWalker(e.m_tree);
		e.m_tree=simp(e.m_tree);
// 		objectWalker(e.m_tree);
		return e;
	} else {
		m_err << i18n("Must specify an operation");
		return Expression();
	}
}

Cn Analitza::calculate()
{
	if(m_exp.isCorrect())
		return calc(m_exp.m_tree);
	else {
		m_err << i18n("Must specify an operation");
		return Cn(0.);
	}
}

Object* Analitza::eval(const Object* branch, bool resolve)
{
	Q_ASSERT(branch && branch->type()!=Object::none);
	Object *ret=0;
	//Won't calc() so would be a good idea to have it simplified
	if(branch->isContainer()) {
		const Container* c = (Container*) branch;
		Operator op = c->firstOperator();
		if(c->containerType()==Object::declare) {
			calc(c);
			ret = new Cn(0.); //FIXME need to have something to return
		} else switch(op.operatorType()) {
			case Object::diff:
				ret = derivative("x", c->m_params[1]);
				break;
			case Object::onone:
				ret = eval(c->m_params[0], resolve);
				break;
			default: { //FIXME: Should we replace the function? the only problem appears if undeclared var in func
				const Container *c = (Container*) branch;
				Operator op(c->firstOperator());
				if(op.operatorType()==Object::function && c->containerType()==Object::apply && op.isBounded()) {
					//it is a function. I'll take only this case for the moment
					const Container *cbody = c;
					QStringList bvars;
					if(op.operatorType()==Object::function) {
						Ci *func= (Ci*) c->m_params[0];
						Object* body= (Object*) m_vars->value(func->name()); //body is the value
						if(body->type()!=Object::container) { //if it is a value variable
							ret = eval(body, resolve);
							break;
						}
						cbody = (Container*) body;
						
						QList<Object*>::const_iterator bv = cbody->m_params.begin();
						for(; bv!=cbody->m_params.end(); ++bv) {
							const Container *ibv;
							if((*bv)->isContainer()) {
								ibv = (Container*) *bv;
								if(ibv->containerType()==Object::bvar) {
									const Ci* ivar;
									if(ibv->m_params[0]) {
										ivar = (Ci*) ibv->m_params[0];
										bvars.append(ivar->name());
									}
								}
							}
							
							QStringList::const_iterator iBvars = bvars.constBegin();
							int i=0;
							for(; iBvars!=bvars.constEnd(); ++iBvars)
								m_vars->stack(*iBvars, c->m_params[++i]);
						}
					}
					
			
					QList<Object*>::const_iterator fval = cbody->firstValue();
					ret = eval(*fval, resolve);
			
					QStringList::const_iterator iBvars(bvars.constBegin());
					for(; iBvars!=bvars.constEnd(); ++iBvars)
						m_vars->destroy(*iBvars);
					
					if(op.operatorType()!=Object::function) {
						Container *nc = new Container(c);
						QList<Object*>::iterator fval = nc->firstValue();
						delete *fval;
						*fval=ret;
						ret = nc;
					}
				} else {
					Container *r = new Container(c);
					QList<Object*>::iterator it(r->firstValue());
					for(; it!=r->m_params.end(); ++it)
						*it = eval(*it, resolve);
					ret=r;
				}
			} break;
		}
	} else if(resolve && branch->type()==Object::variable) {
		Ci* var=(Ci*) branch;
		
		if(m_vars->contains(var->name()) && m_vars->value(var->name()))
			ret = eval(m_vars->value(var->name()), resolve);
	}
	if(!ret)
		return Expression::objectCopy(branch);
	
	return ret;
}

Object* Analitza::derivative(const QString &var, const Object* o)
{
	Q_ASSERT(o);
	Object *ret=0;
	const Container *c=0;
	if(o->type()!=Object::oper && !hasVars(o, var))
		ret = new Cn(0.);
	else switch(o->type()) {
		case Object::container:
			c=(Container*) o;
			if(c->containerType()==Object::apply)
				ret = derivative(var, (Container*) o);
			else {
				Container *cret = new Container(c->containerType());
				QList<Object*>::const_iterator it = c->m_params.begin(), end=c->m_params.end();
				for(; it!=end; it++) {
					cret->m_params.append(derivative(var, *it));
				}
				ret = cret;
			}
				
			break;
		case Object::variable:
			ret = new Cn(1.);
			break;
		case Object::value:
		case Object::oper:
		case Object::none:
			break;
	}
	return ret;
}

Object* Analitza::derivative(const QString &var, const Container *c)
{
	Operator op = c->firstOperator();
	switch(op.operatorType()) {
		case Object::minus:
		case Object::plus: {
			Container *r= new Container(Object::apply);
			r->m_params.append(new Operator(op));
			
			QList<Object*>::const_iterator it(c->firstValue());
			for(; it!=c->m_params.end(); ++it)
				r->m_params.append(derivative(var, *it));
			
			return r;
		} break;
		case Object::times: {
			Container *nx = new Container(Object::apply);
			nx->m_params.append(new Operator(Object::plus));
			
			QList<Object*>::const_iterator it(c->firstValue());
			for(;it!=c->m_params.end(); ++it) {
				Container *neach = new Container(Object::apply);
				neach->m_params.append(new Operator(Object::times));
				
				QList<Object*>::const_iterator iobj(c->firstValue());
					for(; iobj!=c->m_params.end(); ++iobj) {
					Object *o;
					o=Expression::objectCopy(*iobj);
					if(iobj==it)
						o=derivative(var, o);
					
					neach->m_params.append(o);
				}
				nx->m_params.append(neach);
			}
			return nx;
		} break;
		case Object::power: {
			if(hasVars(c->m_params[2], var)) {
				//TODO: http://en.wikipedia.org/wiki/Table_of_derivatives
				//else [if f(x)**g(x)] -> (e**(g(x)*ln f(x)))'
				Container *nc = new Container(Object::apply);
				nc->m_params.append(new Operator(Object::times));
				nc->m_params.append(Expression::objectCopy(c));
				Container *nAss = new Container(Object::apply);
				nAss->m_params.append(new Operator(Object::plus));
				nc->m_params.append(nAss);
				
				Container *nChain1 = new Container(Object::apply);
				nChain1->m_params.append(new Operator(Object::times));
				nChain1->m_params.append(derivative(var, Expression::objectCopy(c->m_params[1])));
				
				Container *cDiv = new Container(Object::apply);
				cDiv->m_params.append(new Operator(Object::divide));
				cDiv->m_params.append(Expression::objectCopy(c->m_params[0]));
				cDiv->m_params.append(Expression::objectCopy(c->m_params[1]));
				nChain1->m_params.append(cDiv);
				
				Container *nChain2 = new Container(Object::apply);
				nChain2->m_params.append(new Operator(Object::times));
				nChain2->m_params.append(derivative(var, Expression::objectCopy(c->m_params[2])));
				
				Container *cLog = new Container(Object::apply);
				cLog->m_params.append(new Operator(Object::ln));
				cLog->m_params.append(Expression::objectCopy(c->m_params[1]));
				nChain2->m_params.append(cLog);
				
				nAss->m_params.append(nChain1);
				nAss->m_params.append(nChain2);
				return nc;
			} else {
				Container *cx = new Container(Object::apply);
				cx->m_params.append(new Operator(Object::times));
				cx->m_params.append(Expression::objectCopy(c->m_params[2]));
				cx->m_params.append(derivative(var, Expression::objectCopy(c->m_params[1])));
				Container* nc= new Container(c);
				cx->m_params.append(nc);
				
				Container *degree = new Container(Object::apply);
				degree->m_params.append(new Operator(Object::minus));
				degree->m_params.append(Expression::objectCopy(c->m_params[2]));
				degree->m_params.append(new Cn(1.));
				nc->m_params[2]=degree;
				return cx;
			}
		} break;
		case Object::sin: {
			Container *ncChain = new Container(Object::apply);
			ncChain->m_params.append(new Operator(Object::times));
			ncChain->m_params.append(derivative(var, *c->firstValue()));
			Container *nc = new Container(Object::apply);
			nc->m_params.append(new Operator(Object::cos));
			nc->m_params.append(Expression::objectCopy(*c->firstValue()));
			ncChain->m_params.append(nc);
			return ncChain;
		} break;
		case Object::cos: {
			Container *ncChain = new Container(Object::apply);
			ncChain->m_params.append(new Operator(Object::times));
			ncChain->m_params.append(derivative(var, *c->firstValue()));
			
			Container *nc = new Container(Object::apply);
			nc->m_params.append(new Operator(Object::sin));
			nc->m_params.append(Expression::objectCopy(*c->firstValue()));
			Container *negation = new Container(Object::apply);
			negation->m_params.append(new Operator(Object::minus));
			negation->m_params.append(nc);
			ncChain->m_params.append(negation);
			return ncChain;
		} break;
		case Object::tan: {
			Container *ncChain = new Container(Object::apply);
			ncChain->m_params.append(new Operator(Object::divide));
			ncChain->m_params.append(derivative(var, *c->firstValue()));
			
			Container *nc = new Container(Object::apply);
			nc->m_params.append(new Operator(Object::power));
			
			Container *lilcosine = new Container(Object::apply);
			lilcosine->m_params.append(new Operator(Object::cos));
			lilcosine->m_params.append(Expression::objectCopy(*c->firstValue()));
			nc->m_params.append(lilcosine);
			nc->m_params.append(new Cn(2.));
			ncChain->m_params.append(nc);
			return ncChain;
		} break;
		case Object::divide: {
			Object *f, *g; //referring to f/g
			f=*c->firstValue();
			g=*(c->firstValue()+1);
			
			Container *nc = new Container(Object::apply);
			nc->m_params.append(new Operator(Object::divide));
			
			Container *cmin = new Container(Object::apply);
			cmin->m_params.append(new Operator(Object::minus));
			
			Container *cmin1 =new Container(Object::apply);
			cmin1->m_params.append(new Operator(Object::times));
			cmin1->m_params.append(derivative(var, Expression::objectCopy(f)));
			cmin1->m_params.append(Expression::objectCopy(g));
			cmin->m_params.append(cmin1);
			nc->m_params.append(cmin);
			
			Container *cmin2 =new Container(Object::apply);
			cmin2->m_params.append(new Operator(Object::times));
			cmin2->m_params.append(Expression::objectCopy(f));
			cmin2->m_params.append(derivative(var, Expression::objectCopy(g)));
			cmin->m_params.append(cmin2);
			
			Container *cquad = new Container(Object::apply);
			cquad->m_params.append(new Operator(Object::power));
			cquad->m_params.append(Expression::objectCopy(g));
			cquad->m_params.append(new Cn(2.));
			nc->m_params.append(cquad);
			
			qDebug() << "iei!" << cmin->toString();
			return nc;
		} break;
		case Object::ln: {
			Container *nc = new Container(Object::apply);
			nc->m_params.append(new Operator(Object::divide));
			nc->m_params.append(derivative(var, Expression::objectCopy(*c->firstValue())));
			nc->m_params.append(Expression::objectCopy(*c->firstValue()));
			return nc;
		} break;
		default: {
			Container* obj = new Container(Object::apply);
			obj->m_params.append(new Operator(Object::diff));
			obj->m_params.append(Expression::objectCopy(c));
			return obj;
		}
	}
	return 0;
}

Cn Analitza::calc(const Object* root)
{
	Q_ASSERT(root && root->type()!=Object::none);
	Cn ret=Cn(0.);
	Ci *a;
	
	switch(root->type()) {
		case Object::container:
			ret = operate((Container*) root);
			break;
		case Object::value:
			ret=(Cn*) root;
			break;
		case Object::variable:
			a=(Ci*) root;
			
			if(m_vars->contains(a->name()))
				ret = calc(m_vars->value(a->name()));
			else if(a->isFunction())
				m_err << i18n("The function <em>%1</em> does not exist", a->name());
			else
				m_err << i18n("The variable <em>%1</em> does not exist", a->name());
			
			break;
		case Object::oper:
		default:
			break;
	}
	return ret;
}

Cn Analitza::operate(const Container* c)
{
	Q_ASSERT(c);
	Operator *op=0;
	Cn ret(0.);
	QList<Cn> numbers;
	
	if(c->containerType() > 100)
		qDebug() << "wow";
	
	if(c->m_params.isEmpty()) {
		m_err << i18n("Empty container: %1", c->containerType());
		return Cn(0.);
	}
	
	if(c->m_params[0]->type() == Object::oper)
		op = (Operator*) c->m_params[0];
	
	if(op!= 0 && op->operatorType()==Object::sum)
		ret = sum(*c);
	else if(op!= 0 && op->operatorType()==Object::product)
		ret = product(*c);
	else switch(c->containerType()) { //TODO: Diffs should be implemented here.
		case Object::apply:
		case Object::math:
		case Object::bvar:
		case Object::uplimit:
		case Object::downlimit:
		{
			if(c->isEmpty()) {
				m_err << i18n("Container without values found.");
			} else if(c->m_params[0]->type() == Object::variable) {
				Ci* var= (Ci*) c->m_params[0];
				
				if(var->isFunction())
					ret = func(c);
				else
					ret = calc(c->m_params[0]);
			} else {
				QList<Object*>::const_iterator it = c->firstValue();
				for(; it!=c->m_params.end(); it++) {
					Q_ASSERT((*it)!=0);
					if((*it)->type() != Object::oper)
						numbers.append(calc(*it));
				}
				
				if(op==0) {
					ret = numbers.first();
				} else if(op->nparams()>-1 && numbers.count()!=op->nparams() && op->operatorType()!=Object::minus) {
					m_err << i18n("Too much operators for <em>%1</em>", op->operatorType());
					ret = Cn(0.);
				} else if(numbers.count()>=1 && op->type()==Object::oper) {
					if(numbers.count()>=2) {
						QList<Cn>::iterator it = numbers.begin();
						ret = *it;
						
						++it;
						for(; it != numbers.end(); ++it)
							reduce(op->operatorType(), &ret, *it, false);
						
					} else {
						ret=numbers.first();
						reduce(op->operatorType(), &ret, 0., true);
					}
				} else {
					ret = numbers.first();
				}
			}
		}
			break;
		case Object::declare:
		{
			if(c->m_params.count()<=1) {
				m_err << i18n("Need a var name and a value");
				return Cn(0.);
			}
			
			Ci *var = (Ci*) c->m_params[0];
			
			switch(c->m_params[1]->type()) {
				case Object::variable:
					m_vars->modify(var->name(), new Ci(c->m_params[1]));
					break;
				case Object::value:
					m_vars->modify(var->name(), new Cn(c->m_params[1]));
					break;
				case Object::oper:
					m_vars->modify(var->name(), new Operator(c->m_params[1]));
					break;
				case Object::container:
					m_vars->modify(var->name(), new Container(c->m_params[1]));
					break;
				case Object::none:
					m_err << i18n("Unvalid var type");
					break;
			}
		} break;
		case Object::lambda:
			ret = calc(*c->firstValue());
			break;
		case Object::cnone:
			break;
	}
	return ret;
}

Cn Analitza::sum(const Container& n)
{
	Cn ret(.0), *c;
	QString var= n.bvarList()[0];
	double ul= Expression::uplimit(n).value();
	double dl= Expression::downlimit(n).value();
	
	m_vars->contains(var);
	m_vars->stack(var, new Cn(0.));
	c = (Cn*) m_vars->value(var);
	
	for(double a = dl; a<=ul; a++){
		*c = a;
		reduce(Object::plus, &ret, calc(n.m_params[4]), false);
	}
	
	m_vars->destroy(var);
	
	return ret;
}

Cn Analitza::product(const Container& n)
{
	Cn ret(1.), *c;
	QString var= n.bvarList()[0];
	double ul= Expression::uplimit(n).value();
	double dl= Expression::downlimit(n).value();
	
	m_vars->stack(var, new Cn(0.));
	c = (Cn*) m_vars->value(var);
	
	for(double a = dl; a<=ul; a++){
		*c = a;
		reduce(Object::times, &ret, calc(n.m_params[4]), false);
	}
	m_vars->destroy(var);
	
	return ret;
}

bool Analitza::isFunction(const Ci& func) const
{
	if(!m_vars->contains(func.name()))
		return false;
	
	Container *c = (Container*) m_vars->value(func.name());
	return (c && c->type()==Object::container && c->containerType() == Object::lambda);
}

Cn Analitza::func(const Container& n)
{
	Cn ret(.0);
	Ci funct(n.m_params[0]);
	
	if(funct.type()!=Object::variable || !funct.isFunction() || !m_vars->contains(funct.name())) {
		m_err << i18n("The function <em>%1</em> does not exist", funct.name());
		return ret;
	}
	
	if(!isFunction(funct)) {
		m_err << i18n("<em>%1</em> is not a function", funct.name());
		return ret;
	}
	
	Container *function = (Container*) m_vars->value(funct.name());
	
	QStringList var = function->bvarList();
	
	for(int i=0; i<var.count(); i++) {
		m_vars->stack(var[i], n.m_params[i+1]);
	}
	
	ret=calc(function->m_params[var.count()]);
	
	for(int i=0; i<var.count(); i++) {
		m_vars->destroy(var[i]);
	}
	
	return ret;
}

void Analitza::reduce(enum Object::OperatorType op, Cn *ret, Cn oper, bool unary)
{
	int residu;
	double a=ret->value(), b=oper.value(), c;
	bool boolean=false;
	
	switch(op) {
		case Object::plus:
			a += b;
			break;
		case Object::times:
			a *= b;
			break;
		case Object::divide:
			a /=b;
			break;
		case Object::minus:
			a = unary ? -a : a-b;
			break;
		case Object::power:
			a = pow(a, b);
			break;
		case Object::rem:
			if(floor(b)!=0.)
				a = static_cast<int>(floor(a)) % static_cast<int>(floor(b));
			else
				m_err << i18n("Cannot calculate the <em>reminder</em> of 0.");
			break;
		case Object::quotient:
			a = floor(a / b);
			break;
		case Object::factorof:
			if(floor(b)!=0.)
				a = (((int)a % (int)b)==0) ? 1.0 : 0.0;
			else {
				a = 0.;
				m_err << i18n("Cannot calculate the <em>factor</em> of 0.");
			}
			boolean = true;
			break;
		case Object::factorial:
			b = floor(a);
			for(a=1.; b>1.; b--)
				a*=b;
			break;
		case Object::sin:
			a=sin(a);
			break;
		case Object::cos:
			a=cos(a);
			break;
		case Object::tan:
			a=tan(a);
			break;
		case Object::sec:
			a=1./cos(a);
			break;
		case Object::csc:
			a=1./sin(a);
			break;
		case Object::cot:
			a=1./tan(a);
			break;
		case Object::sinh:
			a=sinh(a);
			break;
		case Object::cosh:
			a=cosh(a);
			break;
		case Object::tanh:
			a=tanh(a);
			break;
		case Object::sech:
			a=1.0/cosh(a);
			break;
		case Object::csch:
			a=1.0/sinh(a);
			break;
		case Object::coth:
			a=cosh(a)/sinh(a);
			break;
		case Object::arcsin:
			a=asin(a);
			break;
		case Object::arccos:
			a=acos(a);
			break;
		case Object::arctan:
			a=acos(a);
			break;
		case Object::arccot:
			a=log(a+pow(a*a+1., 0.5));
			break;
		case Object::arcsinh:
			a=0.5*(log(1.+1./a)-log(1.-1./a));
			break;
		case Object::arccosh:
			a=log(a+sqrt(a-1.)*sqrt(a+1.));
			break;
	// 	case Object::arccsc:
	// 	case Object::arccsch:
	// 	case Object::arcsec:
	// 	case Object::arcsech:
	// 	case Object::arcsinh:
	// 	case Object::arctanh:
		case Object::exp:
			a=exp(a);
			break;
		case Object::ln:
			a=log(a);
			break;
		case Object::log:
			a=log10(a);
			break;
		case Object::abs:
			a= a>=0. ? a : -a;
			break;
		//case Object::conjugate:
		//case Object::arg:
		//case Object::real:
		//case Object::imaginary:
		case Object::floor:
			a=floor(a);
			break;
		case Object::ceiling:
			a=ceil(a);
			break;
		case Object::min:
			a= a < b? a : b;
			break;
		case Object::max:
			a= a > b? a : b;
			break;
		case Object::gt:
			a= a > b? 1.0 : 0.0;
			boolean=true;
			break;
		case Object::lt:
			a= a < b? 1.0 : 0.0;
			boolean=true;
			break;
		case Object::eq:
			a= a == b? 1.0 : 0.0;
			boolean=true;
			break;
		case Object::approx:
			a= fabs(a-b)<0.001? 1.0 : 0.0;
			boolean=true;
			break;
		case Object::neq:
			a= a != b? 1.0 : 0.0;
			boolean=true;
			break;
		case Object::geq:
			a= a >= b? 1.0 : 0.0;
			boolean=true;
			break;
		case Object::leq:
			a= a <= b? 1.0 : 0.0;
			boolean=true;
			break;
		case Object::_and:
			a= a && b? 1.0 : 0.0;
			boolean=true;
			break;
		case Object::_not:
			a=!a;
			boolean = true;
			break;
		case Object::_or:
			a= a || b? 1.0 : 0.0;
			boolean = true;
			break;
		case Object::_xor:
			a= (a || b) && !(a&&b)? 1.0 : 0.0;
			boolean = true;
			break;
		case Object::implies:
			a= (a && !b)? 0.0 : 1.0;
			boolean = true;
			break;
		case Object::gcd: //code by michael cane aka kiko :)
			while (b > 0.) {
				residu = (int) floor(a) % (int) floor(b);
				a = b;
				b = residu;
			}
			break;
		case Object::lcm: //code by michael cane aka kiko :)
			c=a*b;
			while (b > 0.) {
				residu = (int) floor(a) % (int) floor(b);
				a = b;
				b = residu;
			}
			a=(int)c/(int)a;
			break;
		case Object::root:
			a = b==2.0 ? sqrt(a) : pow(a, 1.0/b);
			break;
			
		default:
			m_err << i18n("The operator <em>%1</em> has not been implemented", op);
			break;
	}
// 	
	ret->setValue(a);
}

QStringList Analitza::bvarList() const //FIXME: if
{
	Q_ASSERT(m_exp.m_tree);
	Container *c = (Container*) m_exp.m_tree;
	if(c!=0 && c->type()==Object::container) {
		c = (Container*) c->m_params[0];
		
		if(c->type()==Object::container)
			return c->bvarList();
	}
	return QStringList();
	
}

/////////////////////////////////////////////
/////////////////////////////////////////////
/////////////////////////////////////////////

void Analitza::simplify()
{
	if(m_exp.isCorrect())
		m_exp.m_tree = simp(m_exp.m_tree);
}

void Analitza::levelOut(Container *c, Container *ob, QList<Object*>::iterator &pos)
{
	QList<Object*>::iterator it = ob->firstValue();
	for(; it!=ob->m_params.end();) {
		pos=c->m_params.insert(pos, *it);
		pos++;
		it=ob->m_params.erase(it);
	}
}

Object* Analitza::simp(Object* root)
{
	Q_ASSERT(root && root->type()!=Object::none);
	Object* aux=0;
	if(!hasVars(root)) {
		if(root->type()!=Object::value && root->type() !=Object::oper) {
			aux = root;
			root = new Cn(calc(root));
			delete aux;
		}
	} else if(root->isContainer()) {
		Container *c= (Container*) root;
		QList<Object*>::iterator it;
		Operator o = c->firstOperator();
		bool d;
		switch(o.operatorType()) {
			case Object::times:
				for(it=c->firstValue(); c->m_params.count()>1 && it!=c->m_params.end();) {
					d=false;
					*it = simp(*it);
					if((*it)->isContainer()) {
						Container *intr = (Container*) *it;
						if(intr->firstOperator()==o.operatorType()) {
							levelOut(c, intr, it);
							d=true;
						}
					}
					
					if(!d && (*it)->type() == Object::value) {
						Cn* n = (Cn*) (*it);
						if(n->value()==1. && c->m_params.count()>2) { //1*exp=exp
							d=true;
						} else if(n->value()==0.) { //0*exp=0
							delete root;
							root = new Cn(0.);
							return root;
						}
					}
					
					if(!d)
						++it;
					else {
						delete *it;
						it = c->m_params.erase(it);
					}
				}
				
				if(c->isUnary()) {
					Container *aux=c;
					root=*c->firstValue();
					*aux->firstValue()=0;
					delete aux;
				} else {
					simpScalar(c);
					simpPolynomials(c);
					
					if(c->firstValue()==c->m_params.end()) {
						delete root;
						root = new Cn(0.);
					}
				}
				break;
			case Object::minus:
			case Object::plus: {
				Object *f=0;
				bool somed=false;
				for(it=c->firstValue(); it!=c->m_params.end();) {
					*it = simp(*it);
					if(f==0) f=*it;
					d=false;
					if((*it)->isContainer()) {
						Container *intr = (Container*) *it;
						if(intr->firstOperator()==Object::plus) {
							levelOut(c, intr, it);
							d=true;
						}
					}
					
					if((*it)->type() == Object::value) {
						Cn* n = (Cn*) (*it);
						if(n->value()==0.) //0+-exp=exp
							d=true;
					}
					
					if(!d)
						++it;
					else {
						somed=true;
						delete *it;
						it = c->m_params.erase(it);
					}
				}
				
				if(c->isUnary() && c->firstOperator()==Object::plus) {
					Container *aux=c;
					root=*c->firstValue();
					*aux->firstValue()=0;
					delete aux;
					c=0;
				} else if(somed && c->isUnary() && c->firstOperator()==Object::minus && *c->firstValue()==f) {
					Container *aux=c;
					root=*c->firstValue();
					*aux->firstValue()=0;
					delete aux;
					break;
				} else {
					simpScalar(c);
					simpPolynomials(c);
				}
				
				if(c && c->firstValue()==c->m_params.end()) {
					delete root;
					root = new Cn(0.);
				}
			} break;
			case Object::power: {
				c->m_params[1] = simp(c->m_params[1]);
				c->m_params[2] = simp(c->m_params[2]);
				
				if(c->m_params[2]->type()==Object::value) {
					Cn *n = (Cn*) c->m_params[2];
					if(n->value()==0.) { //0*exp=0
						delete root;
						root = new Cn(1.);
						break;
					} else if(n->value()==1.) { 
						root = c->m_params[1];
						delete c->m_params[2];
						c->m_params.clear();
						delete c;
						break;
					}
				}
				
				if(c->m_params[1]->isContainer()) {
					Container *cp = (Container*) c->m_params[1];
					if(cp->firstOperator()==Object::power) {
						c->m_params[1] = Expression::objectCopy(cp->m_params[1]);
						
						Container *cm = new Container(Object::apply);
						cm->m_params.append(new Operator(Object::times));
						cm->m_params.append(Expression::objectCopy(c->m_params[2]));
						cm->m_params.append(Expression::objectCopy(cp->m_params[2]));
						c->m_params[2] = cm;
						delete cp;
						c->m_params[2]=simp(c->m_params[2]);
					}
				}
			} break;
			case Object::ln:
				if(c->m_params[1]->type()==Object::variable) {
					Ci *val = (Ci*) c->m_params[1];
					if(val->name()=="e") {
						delete root;
						root = new Cn(1.);
						break;
					}
				}
			default:
				it = c->firstValue();
				
				for(; it!=c->m_params.end(); it++)
					*it = simp(*it);
				break;
		}
	}
	return root;
}


void Analitza::simpScalar(Container * c)
{
	Cn value(0.), *aux;
	Operator o = c->firstOperator();
	bool d, changed=false, sign=true;
	
	QList<Object*>::iterator i = c->firstValue();
	
	for(; i!=c->m_params.end();) {
		d=false;
		
		if((*i)->type()==Object::value) {
			aux = (Cn*) *i;
// 			if(!changed && o.operatorType()==Object::minus && i!=c->firstValue())
// 				aux->negate();
			
			if(changed)
				reduce(o.operatorType(), &value, *aux, false);
			else
				value=*aux;
			d=true;
		} /*else if((*i)->isContainer() && o.operatorType()==Object::times) {
			Container *m = (Container*) *i;
			
			if(m->firstOperator()==Object::minus && m->isUnary()) {
				Object* aux = Expression::objectCopy(*m->firstValue());
				delete *m->firstValue();
				*i = aux;
				sign = !sign;
				changed=true;
			}
		}*/
		
		if(d) {
			delete *i;
			changed=true;
			i = c->m_params.erase(i);
		} else
			++i;
	}
	
	if(changed) {
		bool found=false;
		i=c->firstValue();
		for(; !found && i!=c->m_params.end(); ++i) {
			if((*i)->type()==Object::container) {
				Container *c1 = (Container*) *i;
				if(c1->containerType()==Object::apply)
					found=true;
				
			} else if((*i)->type()==Object::value || (*i)->type()==Object::variable) {
				found=true;
			}
		}
		
		if(!sign)
			value.negate();
		
		if(found && value.value()!=0)
			switch(o.operatorType()) {
				case Object::minus:
				case Object::plus:
					c->m_params.append(new Cn(value));
					break;
				default:
					c->m_params.insert(c->firstValue(), new Cn(value));
			}
	}
	
	return;
}

void Analitza::simpPolynomials(Container* c)
{
	Q_ASSERT(c!=0 && c->type()==Object::container);
	QList<QPair<double, Object*> > monos;
	Operator o(c->firstOperator());
	bool sign=true;
	
	QList<Object*>::iterator it(c->firstValue());
	for(; it!=c->m_params.end(); ++it) {
		Object *o2=*it;
		QPair<double, Object*> imono;
		bool ismono=false;
		
		if(o2->type() == Object::container) {
			Container *cx = (Container*) o2;
			if(cx->firstOperator()==o.multiplicityOperator() && cx->m_params.count()==3) {
				bool valid=false;
				int scalar=-1, var=-1;
				
				if(cx->m_params[1]->type()==Object::value) {
					scalar=1;
					var=2;
					valid=true;
				} else if(cx->m_params[2]->type()==Object::value) {
					scalar=2;
					var=1;
					valid=true;
				}
				
				if(valid) {
					Cn* sc= (Cn*) cx->m_params[scalar];
					imono.first = sc->value();
					imono.second = cx->m_params[var];
					
					ismono=true;
				}
			} else if(cx->firstOperator()==Object::minus && cx->isUnary()) {
				if(o.operatorType()==Object::plus) {
					//detecting -x as QPair<-1, o>
					imono.first = -1.;
					imono.second = *cx->firstValue();
					ismono=true;
				} else if(o.operatorType()==Object::times) {
					imono.first = 1.;
					imono.second = *cx->firstValue();
					ismono=true;
					sign = !sign;
				}
			}
		}
		
		if(!ismono) {
			imono.first = 1.;
			imono.second = Expression::objectCopy(o2);
		}
		
		if(o.operatorType()!=Object::times && imono.second->isContainer()) {
			Container *m = (Container*) imono.second;
			if(m->firstOperator()==Object::minus && m->isUnary()) {
				imono.second = *m->firstValue();
				imono.first *= -1.;
			}
		}
		
		bool found = false;
		QList<QPair<double, Object*> >::iterator it1(monos.begin());
		for(; it1!=monos.end(); ++it1) {
			Object *o1=it1->second, *o2=imono.second;
			if(o2->type()!=Object::oper && Container::equalTree(o1, o2)) {
				found = true;
				break;
			}
		}
		
		if(found) {
			if(o.operatorType()==Object::minus && it!=c->firstValue())
				imono.first= -imono.first;
			it1->first += imono.first;
		} else {
			imono.second = Expression::objectCopy(imono.second);
			monos.append(imono);
		}
	}
	
	qDeleteAll(c->m_params);
	c->m_params.clear();
	
	QList<QPair<double, Object*> >::iterator i=monos.begin();
	c->m_params.append(new Operator(o));
	for(; i!=monos.end(); ++i) {
		if(i->first==0.) {
		} else if(i->first==1.) {
			c->m_params.append(i->second);
		} else if(i->first==-1. && (o.operatorType()==Object::plus || o.operatorType()==Object::minus)) {
			Container *cint = new Container(Container::apply);
			cint->m_params.append(new Operator(Object::minus));
			cint->m_params.append(i->second);
			c->m_params.append(cint);
		} else {
			Container *cint = new Container(Container::apply);
			cint->m_params.append(new Operator(o.multiplicityOperator()));
			cint->m_params.append(i->second);
			cint->m_params.append(new Cn(i->first));
			if(o.multiplicityOperator()==Object::times)
			cint->m_params.swap(1,2);
			c->m_params.append(cint);
		}
	}
	if(!sign) {
		Container *cx = (Container*) Expression::objectCopy(c);
		c->m_params.clear();
		c->m_params.append(new Operator(Object::minus));
		c->m_params.append(cx);
	}
}

bool Analitza::hasVars(const Object *o, const QString &var, const QStringList& bvars)
{
	Q_ASSERT(o);
	
	bool r=false;
	switch(o->type()) {
		case Object::variable: {
			Ci *i = (Ci*) o;
			if(!var.isEmpty()) {
				r=i->name()==var;
			} else
				r=true;
			
			if(r && bvars.contains(i->name()))
				r=false;
		}	break;
		case Object::container: {
			Container *c = (Container*) o;
			Object *first = *c->firstValue();
			bool firstFound=false;
			QList<Object*>::iterator it = c->m_params.begin();
			
			QStringList scope=bvars;
			
			for(; !r && it!=c->m_params.end(); it++) {
				if(*it==first)
					firstFound=true;
				
				if(!firstFound && (*it)->isContainer()) {
					Container *cont= (Container*) *it;
					if(cont->containerType()==Object::bvar) {
						Ci* bvar=(Ci*) cont->m_params[0];
						if(bvar->isCorrect())
							scope += bvar->name();
					}
				} else if(firstFound)
					r |= hasVars(*it, var, scope);
			}
			
		} break;
		case Object::none:
		case Object::value:
		case Object::oper:
			r=false;
	}
	
	return r;
}

Object* Analitza::removeDependencies(Object * o) const
{
	Q_ASSERT(o);
	if(o->type()==Object::variable) {
		Ci* var=(Ci*) o;
		if(m_vars->contains(var->name()) && m_vars->value(var->name())) {
			Object *value=Expression::objectCopy(m_vars->value(var->name()));
			Object *no = removeDependencies(value);
			delete o;
			return no;
		}
	} else if(o->type()==Object::container) {
		Container *c = (Container*) o;
		Operator op(c->firstOperator());
		if(c->containerType()==Object::apply && op.isBounded()) { //it is a function
			Container *cbody = c;
			QStringList bvars;
			if(op.operatorType()==Object::function) {
				Ci *func= (Ci*) c->m_params[0];
				Object* body= (Object*) m_vars->value(func->name());
				if(body->type()!=Object::container)
					return body;
				cbody = (Container*) body;
			}
			
			if(op.operatorType()==Object::function) {
				QStringList::const_iterator iBvars(bvars.constBegin());
				int i=0;
				for(; iBvars!=bvars.constEnd(); ++iBvars)
					m_vars->stack(*iBvars, c->m_params[++i]);
				delete c;
				c = 0;
			}
			
			QList<Object*>::iterator fval(cbody->firstValue());
			Object *ret= removeDependencies(Expression::objectCopy(*fval));
			
			QStringList::const_iterator iBvars(bvars.constBegin());
			for(; iBvars!=bvars.constEnd(); ++iBvars)
				m_vars->destroy(*iBvars);
			
			
			if(op.operatorType()==Object::function)
				return ret;
			else {
				delete *fval;
				*fval=ret;
				return c;
			}
		} else {
			QList<Object*>::iterator it(c->firstValue());
			for(; it!=c->m_params.end(); ++it)
				*it = removeDependencies(*it);
		}
	}
	return o;
}

void Analitza::setVariables(Variables * v)//FIXME: Copy me!
{
	if(m_vars!=NULL)
		delete m_vars;
	m_vars = v;
}

Expression Analitza::derivative()
{
	/* TODO: bvars, entrer to lambda. */
	Expression exp;
	if(m_exp.isCorrect()) {
		exp.m_tree = derivative("x", m_exp.m_tree);
		exp.m_tree = simp(exp.m_tree);
		
// 		objectWalker(exp.m_tree);
	}
	return exp;
}
