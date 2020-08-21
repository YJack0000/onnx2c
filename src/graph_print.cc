#include "error.h"
#include "graph.h"
#include "util.h"
#include <iostream>

using namespace toC;

void Graph::print_header(std::ostream &dst)
{
	print_file_frontmatter(dst);
}

void Graph::print_source(std::ostream &dst)
{
	print_file_frontmatter(dst);
	dst << std::endl;
	print_includes(dst);
	dst << std::endl;
	print_global_tensors(dst);
	dst << std::endl;
	print_functions(dst);
	dst << std::endl;
	print_interface_function(dst);
}


void Graph::print_file_frontmatter(std::ostream &dst)
{
	dst << "// This file is computer-generated by onnx2c " << std::endl;
	dst << "// (TODO: add creating command line here)" << std::endl;
	dst << "// (TODO: print creation date here )" << std::endl;
	dst << std::endl;
	dst << "// ONNX model:" << std::endl;
	dst << "// produced by " << model.producer_name();
	dst << ", version " << model.producer_version() << std::endl;
	dst << "// Model documentation: " << std::endl;
	// TODO: beware & check for maliciously formatted doc strings!!!
	// (and when you do that, also append "//" to every newlin in the doc_string for nicer printing :)
	dst << "/*" << std::endl << model.doc_string() << std::endl << "*/" << std::endl;
}


void Graph::print_global_tensors(std::ostream &dst)
{
	for( auto t : tensors )
	{
		if( t->generate == false )
			continue;

		dst << "/* " << t->name << "*/" << std::endl;
		dst << "static ";
		print_tensor(dst, t);
		if( t->initialize ) {
			dst << " = "<<std::endl;
			t->print_tensor_initializer(dst);
		}

		dst << ";" << std::endl;
	}
}

/* Prints the "float foo[N][M]" part of a tensor */
void Graph::print_tensor(std::ostream &dst, const Tensor *t)
{
	dst << t->data_type_str() << " ";
	dst << t->cname();
	for( unsigned i : t->data_dim )
		dst << "[" << i << "]";
}

void Graph::print_functions(std::ostream &dst)
{
	for( auto n : nodes ) {
		dst << "static inline void ";
		dst << n->c_name() << "( ";
		for( auto t : n->inputs ){
			print_tensor(dst, t);
			// TODO: print this only N-1 first times
			dst << ", ";
		}
		//TODO: handle multi-output case
		for( auto t: n->outputs)
			print_tensor(dst, t);

		dst << " )";
		dst <<  std::endl << "{" << std::endl;

		n->print(dst);

		dst << "}" << std::endl << std::endl;
	}
}

void Graph::print_includes(std::ostream &dst)
{
	dst << "#include <math.h>" << std::endl;
	dst << "#include <stdint.h>" << std::endl;
	dst << "#include <string.h>" << std::endl;

	dst << "#define MAX(X,Y) ( X > Y ? X : Y)" << std::endl;
}

void Graph::print_interface_function(std::ostream &dst)
{

	// TODO: take the interface function name from the ONNX file name
	dst << "void entry(" ;
	for ( auto i : model.graph().input() ) {
		/* TODO: FIXME: separate input tensors that are initialized
		 * or re-initializable (and therefore count as input), from
		 * the "actual" input data */
		Tensor *t=findTensor(i.name());

		if( t && t->isIO ) {
			print_tensor(dst, t);
			dst << ", ";
		}
	}
	for ( auto i : model.graph().output() ) {
		/* TODO: when there are more than one output, see above for how
		 * inputs are handled */
		Tensor *t = findTensor(i.name());

		if( t && t->isIO ) {
			print_tensor(dst, t);
		}
	}
	dst << ") {" << std::endl;

	// since nodes were resolved from graph inputs in the order there were
	// node inputs resolved, the nodes vector is now sorted in order so that
	// we don't need to check dependancies :)
	for( auto n : nodes )
	{
		dst << "\t" << n->c_name() << "( ";
		for( auto i : n->inputs ) {
			dst << i->cname();
			dst << ", ";
		}
		for( auto i : n->outputs ) {
			dst << i->cname();
		}
		dst << ");" << std::endl;
	}

	dst << "}" << std::endl;
}
