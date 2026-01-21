#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "atlas/parallel/mpi/mpi.h"

namespace atlas4py {
namespace mpi {
    class Comm {
    public:
        Comm() : comm_( atlas::mpi::comm() ) {}
        Comm( eckit::mpi::Comm const& comm ) : comm_( comm ) {}
        int size() const {
            if (size_ < 0) {
                size_ = comm_.size();
            }
            return size_;
        }
        int rank() const {
            if (rank_ < 0) {
                rank_ = comm_.rank();
            }
            return rank_;
        }
        void barrier() const { comm_.barrier(); }
        const std::string& name() const {
            if (name_.empty()) {
                name_ = comm_.name();
            }
            return name_;
        }
    private:
        const eckit::mpi::Comm& comm_;
        mutable int size_ = -1;
        mutable int rank_ = -1;
        mutable std::string name_;
    };
}

namespace py = ::pybind11;
void pybind_submodule_mpi(py::module_ &m) {
    py::module_ m_mpi = m.def_submodule("mpi", "mpi python binding.");

    py::class_<mpi::Comm>( m_mpi, "Comm" )
        .def( py::init<>() )
        .def_property_readonly("size", &mpi::Comm::size )
        .def_property_readonly("rank", &mpi::Comm::rank )
        .def_property_readonly("name", &mpi::Comm::name )
        .def("barrier", &mpi::Comm::barrier );

    m_mpi.def("comm", []() { return mpi::Comm(atlas::mpi::comm()); } );
    m_mpi.def("comm", [](const std::string& name) { return mpi::Comm(atlas::mpi::comm(name)); } );
    m_mpi.def("size", []() { return atlas::mpi::size(); } );
    m_mpi.def("rank", []() { return atlas::mpi::rank(); } );
    m_mpi.def("barrier", []() { atlas::mpi::comm().barrier(); } );
    m_mpi.def("finalize", []() { atlas::mpi::finalize(); } );

    m_mpi.def("push", [](const std::string& name) { atlas::mpi::push(name); } );
    m_mpi.def("pop",  [](const std::string& name) { atlas::mpi::pop(name); } );
    m_mpi.def("pop",  []() { atlas::mpi::pop(); } );
}

}  // namespace atlas4py
