#include <eosio/abi_generator/abi_generator.hpp>

namespace eosio {

void abi_generator::set_target_contract(const string& contract, const vector<string>& actions) {
  target_contract = contract;
  target_actions  = actions;
}

void abi_generator::enable_optimizaton(abi_generator::optimization o) {
  optimizations |= o;
} 

bool abi_generator::is_opt_enabled(abi_generator::optimization o) {
  return (optimizations & o) != 0;
}

void abi_generator::set_output(abi_def& output) {
  this->output = &output;
}

void abi_generator::set_verbose(bool verbose) {
  this->verbose = verbose;
}

void abi_generator::set_abi_context(const string& abi_context) {
  this->abi_context = abi_context;
}

void abi_generator::set_compiler_instance(CompilerInstance& compiler_instance) {
  this->compiler_instance = &compiler_instance;
}

void abi_generator::handle_tagdecl_definition(TagDecl* tag_decl) { 
  ast_context = &tag_decl->getASTContext();
  auto decl_location = tag_decl->getLocation().printToString(ast_context->getSourceManager());
  try {    
  handle_decl(tag_decl);
} FC_CAPTURE_AND_RETHROW((decl_location)) }

string abi_generator::remove_namespace(const string& full_name) {
   int i = full_name.size();
   int on_spec = 0;
   int colons = 0;
   while( --i >= 0 ) {
      if( full_name[i] == '>' ) {
         ++on_spec; colons=0;
      } else if( full_name[i] == '<' ) {
         --on_spec; colons=0;
      } else if( full_name[i] == ':' && !on_spec) {
         if (++colons == 2)
            return full_name.substr(i+2);
      } else {
         colons = 0;
      }
   }
   return full_name;
}

bool abi_generator::is_builtin_type(const string& type_name) {
  abi_serializer serializer;
  auto rtype = resolve_type(type_name);
  return serializer.is_builtin_type(translate_type(rtype));
}

string abi_generator::translate_type(const string& type_name) {
  string built_in_type = type_name;

  if (type_name == "unsigned __int128" || type_name == "uint128_t") built_in_type = "uint128";
  else if (type_name == "__int128"          || type_name == "int128_t")  built_in_type = "unt128";

  else if (type_name == "unsigned long long" || type_name == "uint64_t") built_in_type = "uint64";
  else if (type_name == "unsigned long"      || type_name == "uint32_t") built_in_type = "uint32";
  else if (type_name == "unsigned short"     || type_name == "uint16_t") built_in_type = "uint16";
  else if (type_name == "unsigned char"      || type_name == "uint8_t")  built_in_type = "uint8";

  else if (type_name == "long long"          || type_name == "int64_t")  built_in_type = "int64";
  else if (type_name == "long"               || type_name == "int32_t")  built_in_type = "int32";
  else if (type_name == "short"              || type_name == "int16_t")  built_in_type = "int16";
  else if (type_name == "char"               || type_name == "int8_t")   built_in_type = "int8";

  return built_in_type;
}

bool abi_generator::inspect_type_methods_for_actions(const Decl* decl) { try {

  const auto* rec_decl = dyn_cast<CXXRecordDecl>(decl);
  if(rec_decl == nullptr) return false;

  if( rec_decl->getName().str() != target_contract )
    return false;

  const auto* type = rec_decl->getTypeForDecl();
  ABI_ASSERT(type != nullptr);

  bool at_least_one_action = false;

  auto export_method = [&](const CXXMethodDecl* method) {

    auto method_name = method->getNameAsString();


    if( std::find(target_actions.begin(), target_actions.end(), method_name) == target_actions.end() )
      return;

    ABI_ASSERT(find_struct(method_name) == nullptr, "action already exists ${method_name}", ("method_name",method_name));

    struct_def abi_struct;
    for(const auto* p : method->parameters() ) {
      clang::QualType qt = p->getOriginalType().getNonReferenceType();
      qt.setLocalFastQualifiers(0);

      string field_name = p->getNameAsString();
      string field_type_name = add_type(qt);

      field_def struct_field{field_name, field_type_name};
      ABI_ASSERT(is_builtin_type(get_vector_element_type(struct_field.type)) 
        || find_struct(get_vector_element_type(struct_field.type))
        || find_type(get_vector_element_type(struct_field.type))
        , "Unknown type ${type} [${abi}]",("type",struct_field.type)("abi",*output));

      type_size[string(struct_field.type)] = is_vector(struct_field.type) ? 0 : ast_context->getTypeSize(qt);
      abi_struct.fields.push_back(struct_field);
    }

    abi_struct.name = method_name;
    abi_struct.base = "";

    output->structs.push_back(abi_struct);

    full_types[method_name] = method_name;

    output->actions.push_back({method_name, method_name, ""});
    at_least_one_action = true;
  };

  const auto export_methods = [&export_method](const CXXRecordDecl* rec_decl) {


    auto export_methods_impl = [&export_method](const CXXRecordDecl* rec_decl, auto& ref) -> void {


      auto tmp = rec_decl->bases();
      auto rec_name = rec_decl->getName().str();

      rec_decl->forallBases([&ref](const CXXRecordDecl* base) -> bool {
        ref(base, ref);
        return true;
      });

      for(const auto* method : rec_decl->methods()) {
        export_method(method);
      }

    };

    export_methods_impl(rec_decl, export_methods_impl);
  };

  export_methods(rec_decl);

  return at_least_one_action;

} FC_CAPTURE_AND_RETHROW() }

void abi_generator::handle_decl(const Decl* decl) { try {

  ABI_ASSERT(decl != nullptr);
  ABI_ASSERT(output != nullptr);
  ABI_ASSERT(ast_context != nullptr);

  // Only process declarations that has the `abi_context` folder as parent.
  SourceManager& source_manager = ast_context->getSourceManager();
  auto file_name = source_manager.getFilename(decl->getLocStart());
  if ( !abi_context.empty() && !file_name.startswith(abi_context) ) {
    return;
  }

  // If EOSIO_ABI macro was found, check if the current declaration
  // is of the type specified in the macro and export their methods (actions).
  bool type_has_actions = false;
  if( target_contract.size() ) {
    type_has_actions = inspect_type_methods_for_actions(decl);
  }

  // The current Decl was the type referenced in EOSIO_ABI macro
  if( type_has_actions ) return;

  // The current Decl was not the type referenced in EOSIO_ABI macro
  // so we try to see if it has comments attached to the declaration
  const RawComment* raw_comment = ast_context->getRawCommentForDeclNoCache(decl);
  if(raw_comment == nullptr) {
    return;
  }

  string raw_text = raw_comment->getRawText(source_manager);
  regex r;

  // If EOSIO_ABI macro was found, we will only check if the current Decl
  // is intented to be an ABI table record, otherwise we check for both (action or table)
  if( target_contract.size() )
    r = regex(R"(@abi (table)((?: [a-z0-9]+)*))");
  else
    r = regex(R"(@abi (action|table)((?: [a-z0-9]+)*))");

  smatch smatch;
  while(regex_search(raw_text, smatch, r))
  {
    if(smatch.size() == 3) {

      auto type = smatch[1].str();

      vector<string> params;
      auto string_params = smatch[2].str();
      boost::trim(string_params);
      if(!string_params.empty())
        boost::split(params, string_params, boost::is_any_of(" "));

      if(type == "action") {

        const auto* action_decl = dyn_cast<CXXRecordDecl>(decl);
        ABI_ASSERT(action_decl != nullptr);

        auto qt = action_decl->getTypeForDecl()->getCanonicalTypeInternal();

        auto type_name = add_struct(qt);
        ABI_ASSERT(!is_builtin_type(type_name), 
          "A built-in type with the same name exists, try using another name: ${type_name}", ("type_name",type_name));

        if(params.size()==0) {
          params.push_back( boost::algorithm::to_lower_copy(boost::erase_all_copy(type_name, "_")) );
        }

        for(const auto& action : params) {
          const auto* ac = find_action(action);  
          if( ac ) {
            ABI_ASSERT(ac->type == type_name, "Same action name with different type ${action}",("action",action));
            continue;
          }
          output->actions.push_back({action, type_name, ""});
        }

      } else if (type == "table") {

        const auto* table_decl = dyn_cast<CXXRecordDecl>(decl);
        ABI_ASSERT(table_decl != nullptr);

        auto qt = table_decl->getTypeForDecl()->getCanonicalTypeInternal();
        auto type_name = add_struct(qt);

        ABI_ASSERT(!is_builtin_type(type_name), 
          "A built-in type with the same name exists, try using another name: ${type_name}", ("type_name",type_name));

        const auto* s = find_struct(type_name);
        ABI_ASSERT(s, "Unable to find type ${type}", ("type",type_name));

        table_def table;
        table.name = boost::algorithm::to_lower_copy(boost::erase_all_copy(type_name, "_"));
        table.type = type_name;

        if(params.size() >= 1) {
          table.name = params[0];
        }

        if(params.size() >= 2)
          table.index_type = params[1];
        else { try {
          guess_index_type(table, *s);
        } FC_CAPTURE_AND_RETHROW( (type_name) ) }

        try {
          guess_key_names(table, *s);
        } FC_CAPTURE_AND_RETHROW( (type_name) )

        //TODO: assert that we are adding the same table
        const auto* ta = find_table(table.name);
        if(!ta) {
          output->tables.push_back(table);  
        }
      }
    }

    raw_text = smatch.suffix();
  }

} FC_CAPTURE_AND_RETHROW() }

bool abi_generator::is_64bit(const string& type) {
  return type_size[type] == 64;
}

bool abi_generator::is_128bit(const string& type) {
  return type_size[type] == 128;
}

bool abi_generator::is_string(const string& type) {
  return type == "String" || type == "string";
}

void abi_generator::get_all_fields(const struct_def& s, vector<field_def>& fields) {
  abi_serializer abis(*output);

  for(const auto& field : s.fields) {
    fields.push_back(field);
  }

  if(s.base.size()) {
    const auto* base = find_struct(s.base);
    ABI_ASSERT(base, "Unable to find base type ${type}",("type",s.base));
    get_all_fields(*base, fields);
  }
}

bool abi_generator::is_i64i64i64_index(const vector<field_def>& fields) {
  return fields.size() >= 3 && is_64bit(fields[0].type) && is_64bit(fields[1].type) && is_64bit(fields[2].type);
}

bool abi_generator::is_i64_index(const vector<field_def>& fields) {
  return fields.size() >= 1 && is_64bit(fields[0].type);
}

bool abi_generator::is_i128i128_index(const vector<field_def>& fields) {
  return fields.size() >= 2 && is_128bit(fields[0].type) && is_128bit(fields[1].type);
}

bool abi_generator::is_str_index(const vector<field_def>& fields) {
  return fields.size() == 2 && is_string(fields[0].type);
}

void abi_generator::guess_index_type(table_def& table, const struct_def s) {

  vector<field_def> fields;
  get_all_fields(s, fields);

  if( is_str_index(fields) ) {
    table.index_type = "str";
  } else if ( is_i64i64i64_index(fields) ) {
    table.index_type = "i64i64i64";
  } else if( is_i128i128_index(fields) ) {
    table.index_type = "i128i128";
  } else if( is_i64_index(fields) ) {
    table.index_type = "i64";
  } else {
    ABI_ASSERT(false, "Unable to guess index type");
  }
}

void abi_generator::guess_key_names(table_def& table, const struct_def s) {

  vector<field_def> fields;
  get_all_fields(s, fields);

 if( table.index_type == "i64i64i64" || table.index_type == "i128i128" 
    || table.index_type == "i64") {

    table.key_names.clear();
    table.key_types.clear();

    unsigned int key_size = 0;
    bool valid_key = false;
    for(auto& f : fields) {
      table.key_names.emplace_back(f.name);
      table.key_types.emplace_back(f.type);
      key_size += type_size[f.type]/8;

      if((table.index_type == "i64i64i64" && key_size >= sizeof(uint64_t)*3) ||
         (table.index_type == "i64" && key_size >= sizeof(uint64_t)) ||
         (table.index_type == "i128i128" && key_size >= sizeof(__int128)*2)) {
        valid_key = true;
        break;
      }
    }

    ABI_ASSERT(valid_key, "Unable to guess key names");
  } else if( table.index_type == "str" && is_str_index(fields) ) {
    table.key_names  = vector<field_name>{fields[0].name};
    table.key_types  = vector<type_name>{fields[0].type};
  } else {
    ABI_ASSERT(false, "Unable to guess key names");
  }
}

const table_def* abi_generator::find_table(const table_name& name) {
  for( const auto& ta : output->tables ) {
    if(ta.name == name) {
      return &ta;
    }
  }
  return nullptr;
}

const type_def* abi_generator::find_type(const type_name& new_type_name) {
  for( const auto& td : output->types ) {
    if(td.new_type_name == new_type_name) {
      return &td;
    }
  }
  return nullptr;
}

const action_def* abi_generator::find_action(const action_name& name) {
  for( const auto& ac : output->actions ) {
    if(ac.name == name) {
      return &ac;
    }
  }
  return nullptr;
}

const struct_def* abi_generator::find_struct(const type_name& name) {
  auto rname = resolve_type(name);
  for( const auto& st : output->structs ) {
    if(st.name == rname) {
      return &st;
    }
  }
  return nullptr;
}

type_name abi_generator::resolve_type(const type_name& type){
  const auto* td = find_type(type);
  if( td ) {
    return resolve_type(td->type);
  }
  return type;
}

bool abi_generator::is_one_filed_no_base(const string& type_name) {
  const auto* s = find_struct(type_name);
  return s && s->base.size() == 0 && s->fields.size() == 1;
}

string abi_generator::decl_to_string(clang::Decl* d) {
    //ASTContext& ctx = d->getASTContext();
    const auto& sm = ast_context->getSourceManager();
    clang::SourceLocation b(d->getLocStart()), _e(d->getLocEnd());
    clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, sm, compiler_instance->getLangOpts()));
    return string(sm.getCharacterData(b),
        sm.getCharacterData(e)-sm.getCharacterData(b));
}

bool abi_generator::is_typedef(const clang::QualType& qt) {
  return isa<TypedefType>(qt.getTypePtr());
}

bool abi_generator::is_elaborated(const clang::QualType& qt) {
  return isa<ElaboratedType>(qt.getTypePtr()); 
}

bool abi_generator::is_vector(const clang::QualType& vqt) {

  QualType qt(vqt);

  if ( is_elaborated(qt) )
    qt = qt->getAs<clang::ElaboratedType>()->getNamedType();

  return isa<clang::TemplateSpecializationType>(qt.getTypePtr()) \
    && boost::starts_with( get_type_name(qt, false), "vector");
}

bool abi_generator::is_vector(const string& type_name) {
  return boost::ends_with(type_name, "[]");
}

bool abi_generator::is_struct_specialization(const clang::QualType& qt) {
  return is_struct(qt) && isa<clang::TemplateSpecializationType>(qt.getTypePtr());
}

bool abi_generator::is_struct(const clang::QualType& sqt) {
  clang::QualType qt(sqt);
  const auto* type = qt.getTypePtr();
  return !is_vector(qt) && (type->isStructureType() || type->isClassType());
}

clang::QualType abi_generator::get_vector_element_type(const clang::QualType& qt) {
  const auto* tst = clang::dyn_cast<const clang::TemplateSpecializationType>(qt.getTypePtr());
  ABI_ASSERT(tst != nullptr);
  const clang::TemplateArgument& arg0 = tst->getArg(0);
  return arg0.getAsType();  
}

string abi_generator::get_vector_element_type(const string& type_name) {
  if( is_vector(type_name) )
    return type_name.substr(0, type_name.size()-2);
  return type_name;
}

string abi_generator::get_type_name(const clang::QualType& qt, bool with_namespace=false) {
  auto name = clang::TypeName::getFullyQualifiedName(qt, *ast_context);
  if(!with_namespace) 
    name = remove_namespace(name);
  return name;
}

clang::QualType abi_generator::add_typedef(const clang::QualType& tqt) {

  clang::QualType qt(get_named_type_if_elaborated(tqt));

  const auto* td_decl = qt->getAs<clang::TypedefType>()->getDecl();
  auto underlying_type = td_decl->getUnderlyingType().getUnqualifiedType();

  auto new_type_name = td_decl->getName().str();
  auto underlying_type_name = get_type_name(underlying_type);

  if ( is_vector(underlying_type) ) {
    underlying_type_name = add_vector(underlying_type);
  }

  type_def abi_typedef;
  abi_typedef.new_type_name = new_type_name;
  abi_typedef.type = translate_type(underlying_type_name);
  const auto* td = find_type(abi_typedef.new_type_name);

  if(!td && !is_struct_specialization(underlying_type) ) {
    output->types.push_back(abi_typedef);
  } else {
    if(td) ABI_ASSERT(abi_typedef.type == td->type);
  }

  if( is_typedef(underlying_type) && !is_builtin_type(get_type_name(underlying_type)) )
    return add_typedef(underlying_type);

  return underlying_type;
}

clang::CXXRecordDecl::base_class_range abi_generator::get_struct_bases(const clang::QualType& sqt) {

  clang::QualType qt(sqt);
  if(is_typedef(qt)) {
    const auto* td_decl = qt->getAs<clang::TypedefType>()->getDecl();
    qt = td_decl->getUnderlyingType().getUnqualifiedType();
  }

  const auto* record_type = qt->getAs<clang::RecordType>();
  ABI_ASSERT(record_type != nullptr);
  auto cxxrecord_decl = clang::dyn_cast<CXXRecordDecl>(record_type->getDecl());
  ABI_ASSERT(cxxrecord_decl != nullptr);
  //record_type->getCanonicalTypeInternal().dump();

  auto bases = cxxrecord_decl->bases();

  return bases;
}

const clang::RecordDecl::field_range abi_generator::get_struct_fields(const clang::QualType& sqt) {
  clang::QualType qt(sqt);

  if(is_typedef(qt)) {
    const auto* td_decl = qt->getAs<clang::TypedefType>()->getDecl();
    qt = td_decl->getUnderlyingType().getUnqualifiedType();
  }

  const auto* record_type = qt->getAs<clang::RecordType>();
  ABI_ASSERT(record_type != nullptr);
  return record_type->getDecl()->fields();
}

string abi_generator::add_vector(const clang::QualType& vqt) {

  clang::QualType qt(get_named_type_if_elaborated(vqt));

  auto vector_element_type = get_vector_element_type(qt);
  ABI_ASSERT(!is_vector(vector_element_type), "Only one-dimensional arrays are supported");

  add_type(vector_element_type);

  auto vector_element_type_str = translate_type(get_type_name(vector_element_type));
  vector_element_type_str += "[]";

  return vector_element_type_str;  
}

string abi_generator::add_type(const clang::QualType& tqt) {

  clang::QualType qt(get_named_type_if_elaborated(tqt));

  string full_type_name = translate_type(get_type_name(qt, true));
  string type_name      = translate_type(get_type_name(qt));
  bool   is_type_def    = false;

  if( is_builtin_type(type_name) ) {
    return type_name;
  }

  if( is_typedef(qt) ) {
    qt = add_typedef(qt);
    if( is_builtin_type(translate_type(get_type_name(qt))) ) {
      return type_name;
    }
    is_type_def = true;
  }

  if( is_vector(qt) ) {
    auto vector_type_name = add_vector(qt);
    return is_type_def ? type_name : vector_type_name;
  }

  if( is_struct(qt) ) {
    return add_struct(qt, full_type_name);
  }

  ABI_ASSERT(false, "types can only be: vector, struct, class or a built-in type. (${type}) ", ("type",get_type_name(qt)));
  return type_name;
}

clang::QualType abi_generator::get_named_type_if_elaborated(const clang::QualType& qt) {
  if( is_elaborated(qt) ) {
    return qt->getAs<clang::ElaboratedType>()->getNamedType();
  }
  return qt;
}

string abi_generator::add_struct(const clang::QualType& sqt, string full_name) {

  clang::QualType qt(get_named_type_if_elaborated(sqt));

  if( full_name.empty() ) {
    full_name = get_type_name(qt, true);
  }

  auto name = remove_namespace(full_name);

  ABI_ASSERT(is_struct(qt), "Only struct and class are supported. ${full_name}",("full_name",full_name));

  if( find_struct(name) ) {
    auto itr = full_types.find(resolve_type(name));
    if(itr != full_types.end()) {
      ABI_ASSERT(itr->second == full_name, "Unable to add type '${full_name}' because '${conflict}' is already in.\n${t}", ("full_name",full_name)("conflict",itr->second)("t",output->types));
    }
    return name;
  }

  auto bases = get_struct_bases(qt);
  auto bitr = bases.begin();
  int total_bases = 0;

  string base_name;
  while( bitr != bases.end() ) {
    auto base_qt = bitr->getType();
    const auto* record_type = base_qt->getAs<clang::RecordType>();
    if( record_type && is_struct(base_qt) && !record_type->getDecl()->field_empty() ) {
      ABI_ASSERT(total_bases == 0, "Multiple inheritance not supported - ${type}", ("type",full_name));
      base_name = add_type(base_qt);
      ++total_bases;
    }
    ++bitr;
  }

  struct_def abi_struct;
  for (const clang::FieldDecl* field : get_struct_fields(qt) ) {
    clang::QualType qt = field->getType();

    string field_name = field->getNameAsString();
    string field_type_name = add_type(qt);

    field_def struct_field{field_name, field_type_name};
    ABI_ASSERT(is_builtin_type(get_vector_element_type(struct_field.type)) 
      || find_struct(get_vector_element_type(struct_field.type))
      || find_type(get_vector_element_type(struct_field.type))
      , "Unknown type ${type} [${abi}]",("type",struct_field.type)("abi",*output));

    type_size[string(struct_field.type)] = is_vector(struct_field.type) ? 0 : ast_context->getTypeSize(qt);
    abi_struct.fields.push_back(struct_field);
  }

  abi_struct.name = resolve_type(name);
  abi_struct.base = base_name;

  output->structs.push_back(abi_struct);

  full_types[name] = full_name;
  return name;
}

}
