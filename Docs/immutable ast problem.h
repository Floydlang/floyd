

	//////////////////////////////////////////////////		scope_def_t



	struct scope_def2_immutable_t {
		public: enum etype {
			k_function_scope,
			k_struct_scope,
			k_global_scope,
			k_subscope
		};
		public: etype _type;
		public: type_identifier_t _name;

		public: std::vector<member_t> _members;
		public: std::shared_ptr<executable_t> _executable;
		public: std::shared_ptr<types_collector_t> _types_collector;
		public: type_identifier_t _return_type;
	};

	struct ast_t;
	struct ast_path {
		std::vector<std::string> _path;
	};
	std::pair<ast_t, scope_def2_immutable_t> add_scope_type(const ast_t& ast, const ast_path& path, const std::string& name, const std::shared_ptr<type_def_t>& aaa);
	ast_t replace_scope(const ast_t& ast, const ast_path& path, const scope_def2_immutable_t& new_scope);



	struct ast_i {
		ast_path get_root();
		ast_path get_parent(const ast_path& path);
		const scope_def2_immutable_t get_scope(const ast_path& path);
		ast_t store_scope(const ast_path& path, const scope_def2_immutable_t& scope_def);
	};
	scope_def2_immutable_t add_type(const ast_i& ast, const scope_def2_immutable_t& scope_def, const std::string& name, const std::shared_ptr<type_def_t>& aaa);

	struct ast2_t {
		ast_path get_root() const;
		ast_path get_parent(const ast_path& path) const;
		scope_def2_immutable_t get_scope(const ast_path& path);
		ast2_t store_scope(const ast_path& path, const scope_def2_immutable_t& scope_def);
	};
	ast2_t add_type(const ast2_t& ast, const ast_path& p, const std::string& name, const std::shared_ptr<type_def_t>& aaa);




	//////////////////////////////////////////////////		xxxscope_node_t


	/*
		Idea for non-intrusive scope tree.
	*/

	struct xxxscope_node_t {
		public: bool check_invariant() const {
			return true;
		};

		scope_def_t _scope;
		std::vector<scope_def_t> _children;
	};



