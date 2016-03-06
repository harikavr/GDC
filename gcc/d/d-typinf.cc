// d-typinf.cc -- D frontend for GCC.
// Copyright (C) 2011-2015 Free Software Foundation, Inc.

// GCC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.

// GCC is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with GCC; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/module.h"
#include "dfrontend/mtype.h"
#include "dfrontend/scope.h"
#include "dfrontend/template.h"
#include "dfrontend/declaration.h"
#include "dfrontend/aggregate.h"


// Get the exact TypeInfo for TYPE.

void
genTypeInfo(Type *type, Scope *sc)
{
  if (!Type::dtypeinfo)
    {
      type->error(Loc(), "TypeInfo not found. object.d may be incorrectly installed or corrupt");
      fatal();
    }

  // Do this since not all Type's are merge'd
  Type *t = type->merge2();
  if (!t->vtinfo)
    {
      // Does both 'shared' and 'shared const'
      if (t->isShared())
	t->vtinfo = TypeInfoSharedDeclaration::create(t);
      else if (t->isConst())
	t->vtinfo = TypeInfoConstDeclaration::create(t);
      else if (t->isImmutable())
	t->vtinfo = TypeInfoInvariantDeclaration::create(t);
      else if (t->isWild())
	t->vtinfo = TypeInfoWildDeclaration::create(t);
      else if (t->ty == Tpointer)
	t->vtinfo = TypeInfoPointerDeclaration::create(type);
      else if (t->ty == Tarray)
	t->vtinfo = TypeInfoArrayDeclaration::create(type);
      else if (t->ty == Tsarray)
	t->vtinfo = TypeInfoStaticArrayDeclaration::create(type);
      else if (t->ty == Taarray)
	t->vtinfo = TypeInfoAssociativeArrayDeclaration::create(type);
      else if (t->ty == Tstruct)
	t->vtinfo = TypeInfoStructDeclaration::create(type);
      else if (t->ty == Tvector)
	t->vtinfo = TypeInfoVectorDeclaration::create(type);
      else if (t->ty == Tenum)
	t->vtinfo = TypeInfoEnumDeclaration::create(type);
      else if (t->ty == Tfunction)
	t->vtinfo = TypeInfoFunctionDeclaration::create(type);
      else if (t->ty == Tdelegate)
	t->vtinfo = TypeInfoDelegateDeclaration::create(type);
      else if (t->ty == Ttuple)
	t->vtinfo = TypeInfoTupleDeclaration::create(type);
      else if (t->ty == Tclass)
	{
	  if (((TypeClass *) type)->sym->isInterfaceDeclaration())
	    t->vtinfo = TypeInfoInterfaceDeclaration::create(type);
	  else
	    t->vtinfo = TypeInfoClassDeclaration::create(type);
	}
      else
        t->vtinfo = TypeInfoDeclaration::create(type, 0);

      gcc_assert(t->vtinfo);

      // If this has a custom implementation in rt/typeinfo, then
      // do not generate a COMDAT for it.
      bool builtinTypeInfo = false;
      if (t->isTypeBasic() || t->ty == Tclass)
	builtinTypeInfo = !t->mod;
      else if (t->ty == Tarray)
	{
	  Type *next = t->nextOf();
	  // Strings are so common, make them builtin.
	  builtinTypeInfo = !t->mod
	    && ((next->isTypeBasic() != NULL && !next->mod)
		|| (next->ty == Tchar && next->mod == MODimmutable)
		|| (next->ty == Tchar && next->mod == MODconst));
	}

      if (!builtinTypeInfo)
	{
	  if (sc)
	    {
	      // Find module that will go all the way to an object file
	      Module *m = sc->module->importedFrom;
	      m->members->push(t->vtinfo);
	    }
	  else
	    t->vtinfo->toObjFile();
	}
    }
  // Types aren't merged, but we can share the vtinfo's
  if (!type->vtinfo)
    type->vtinfo = t->vtinfo;

  gcc_assert(type->vtinfo != NULL);
}

Type *
getTypeInfoType(Type *type, Scope *sc)
{
  gcc_assert(type->ty != Terror);
  genTypeInfo(type, sc);
  return type->vtinfo->type;
}

// Bugzilla 14425: TypeInfo_Struct would refer the members of struct
// (e.g. opEquals via xopEquals field), so if it's instantiated in
// speculative context, TypeInfo creation should also be stopped to
// avoid 'unresolved symbol' linker errors.

bool
isSpeculativeType(Type *t)
{
  class SpeculativeTypeVisitor : public Visitor
  {
  public:
    bool result;

    SpeculativeTypeVisitor() : result(false) {}

    void visit(Type *t)
    {
      Type *tb = t->toBasetype();
      if (tb != t)
	tb->accept(this);
    }

    void visit(TypeNext *t)
    {
      if (t->next)
	t->next->accept(this);
    }

    void visit(TypeBasic *) { }

    void visit(TypeVector *t)
    {
      t->basetype->accept(this);
    }

    void visit(TypeAArray *t)
    {
      t->index->accept(this);
      visit((TypeNext *)t);
    }

    void visit(TypeFunction *t)
    {
      visit((TypeNext *)t);
      // Currently TypeInfo_Function doesn't store parameter types.
    }

    void visit(TypeStruct *t)
    {
      StructDeclaration *sd = t->sym;
      if (TemplateInstance *ti = sd->isInstantiated())
	{
	  if (!ti->needsCodegen())
	    {
	      if (ti->minst || sd->requestTypeInfo)
		return;

	      result |= true;
	      return;
	    }
	}
    }

    void visit(TypeClass *) { }

    void visit(TypeTuple *t)
    {
      if (t->arguments)
	{
	  for (size_t i = 0; i < t->arguments->dim; i++)
	    {
	      Type *tprm = (*t->arguments)[i]->type;
	      if (tprm)
		tprm->accept(this);
	      if (result)
		return;
	    }
	}
    }
  };

  SpeculativeTypeVisitor v;
  t->accept(&v);
  return v.result;
}

