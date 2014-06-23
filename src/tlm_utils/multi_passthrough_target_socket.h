/*****************************************************************************

  The following code is derived, directly or indirectly, from the SystemC
  source code Copyright (c) 1996-2011 by all Contributors.
  All Rights reserved.

  The contents of this file are subject to the restrictions and limitations
  set forth in the SystemC Open Source License Version 3.0 (the "License");
  You may not use this file except in compliance with such restrictions and
  limitations. You may obtain instructions on how to receive a copy of the
  License at http://www.systemc.org/. Software distributed by Contributors
  under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
  ANY KIND, either express or implied. See the License for the specific
  language governing rights and limitations under the License.

 *****************************************************************************/
#ifndef __MULTI_PASSTHROUGH_TARGET_SOCKET_H__
#define __MULTI_PASSTHROUGH_TARGET_SOCKET_H__

#include "tlm_utils/multi_socket_bases.h"
#include <sstream>

namespace tlm_utils {

/*
This class implements a trivial multi target socket.
The triviality refers to the fact that the socket does not
do blocking to non-blocking or non-blocking to blocking conversions.

It allows to connect multiple initiators to this socket.
The user has to register callbacks for the fw interface methods
he likes to use. The callbacks are basically equal to the fw interface
methods but carry an additional integer that indicates to which
index of this socket the calling initiator is connected.
*/
template <typename MODULE,
          unsigned int BUSWIDTH = 32,
          typename TYPES = tlm::tlm_base_protocol_types,
          unsigned int N=0
#if !(defined SYSTEMC_VERSION & SYSTEMC_VERSION <= 20050714)
          ,sc_core::sc_port_policy POL = sc_core::SC_ONE_OR_MORE_BOUND
#endif
          >
class multi_passthrough_target_socket: public multi_target_base< BUSWIDTH,
                                                        TYPES,
                                                        N
#if !(defined SYSTEMC_VERSION & SYSTEMC_VERSION <= 20050714)
                                                        ,POL
#endif
                                                        >
                              , public multi_to_multi_bind_base<TYPES>
{

public:

  //typedefs
  //  tlm 2.0 types for nb_transport
  typedef typename TYPES::tlm_payload_type              transaction_type;
  typedef typename TYPES::tlm_phase_type                phase_type;
  typedef tlm::tlm_sync_enum                            sync_enum_type;

  //  typedefs to keep the fn ptr notations short
  typedef sync_enum_type (MODULE::*nb_cb)(int, transaction_type&, phase_type&, sc_core::sc_time&);
  typedef void (MODULE::*b_cb)(int, transaction_type&, sc_core::sc_time&);
  typedef unsigned int (MODULE::*dbg_cb)(int, transaction_type& txn);
  typedef bool (MODULE::*dmi_cb)(int, transaction_type& txn, tlm::tlm_dmi& dmi);

  typedef multi_target_base<BUSWIDTH,
                        TYPES,
                        N
#if !(defined SYSTEMC_VERSION & SYSTEMC_VERSION <= 20050714)
                        ,POL
#endif
                        > base_type;

  typedef typename base_type::base_initiator_socket_type base_initiator_socket_type;

  //CTOR
  multi_passthrough_target_socket()
      : base_type(sc_core::sc_gen_unique_name("multi_passthrough_target_socket"))
      , m_hierarch_bind(0)
      , m_eoe_disabled(false)
      , m_dummy(42)
  {
  }

  //CTOR
  multi_passthrough_target_socket(const char* name)
      : base_type(name)
      , m_hierarch_bind(0)
      , m_eoe_disabled(false)
      , m_dummy(42)
  {
  }

  ~multi_passthrough_target_socket(){
    //clean up everything allocated by 'new'
    for (unsigned int i=0; i<m_binders.size(); i++) delete m_binders[i];
  }

  //simple helpers for warnings an errors to shorten in code notation
  void display_warning(const std::string& text) const {
    std::stringstream s;
    s<<"WARNING in instance "<<base_type::name()<<": "<<text;
    SC_REPORT_WARNING("/OSCI_TLM-2/multi_socket", s.str().c_str());
  }

  void display_error(const std::string& text) const {
    std::stringstream s;
    s<<"ERROR in instance "<<base_type::name()<<": "<<text;
    SC_REPORT_ERROR("/OSCI_TLM-2/multi_socket", s.str().c_str());
  }

  //register callback for nb transport of fw interface
  void register_nb_transport_fw(MODULE* mod,
                              nb_cb cb)
  {
    //if our export hasn't been bound yet (due to a hierarch binding)
    //  we bind it now.
    //We do that here as the user of the target port HAS to bind at least on callback,
    //otherwise the socket was useless. Nevertheless, the target socket may still
    // stay unbound afterwards.
    if (!sc_core::sc_export<tlm::tlm_fw_transport_if<TYPES> >::get_interface())
      sc_core::sc_export<tlm::tlm_fw_transport_if<TYPES> >::bind(m_dummy);

    //warn if there already is a callback
    if (!m_nb_f.empty()){
      display_warning("NBTransport_bw callback already registered.");
      return;
    }

    //set the functor
    m_nb_f.set_function(mod, cb);
  }

  //register callback for b transport of fw interface
  void register_b_transport(MODULE* mod,
                              b_cb cb)
  {
    //if our export hasn't been bound yet (due to a hierarch binding)
    //  we bind it now.
    //We do that here as the user of the target port HAS to bind at least on callback,
    //otherwise the socket was useless. Nevertheless, the target socket may still
    // stay unbound afterwards.
    if (!sc_core::sc_export<tlm::tlm_fw_transport_if<TYPES> >::get_interface())
      sc_core::sc_export<tlm::tlm_fw_transport_if<TYPES> >::bind(m_dummy);

    //warn if there already is a callback
    if (!m_b_f.empty()){
      display_warning("BTransport callback already registered.");
      return;
    }

    //set the functor
    m_b_f.set_function(mod, cb);
  }

  //register callback for debug transport of fw interface
  void register_transport_dbg(MODULE* mod,
                              dbg_cb cb)
  {
    //if our export hasn't been bound yet (due to a hierarch binding)
    //  we bind it now.
    //We do that here as the user of the target port HAS to bind at least on callback,
    //otherwise the socket was useless. Nevertheless, the target socket may still
    // stay unbound afterwards.
    if (!sc_core::sc_export<tlm::tlm_fw_transport_if<TYPES> >::get_interface())
      sc_core::sc_export<tlm::tlm_fw_transport_if<TYPES> >::bind(m_dummy);

    //warn if there already is a callback
    if (!m_dbg_f.empty()){
      display_warning("DebugTransport callback already registered.");
      return;
    }

    //set the functor
    m_dbg_f.set_function(mod, cb);
  }

  //register callback for DMI of fw interface
  void register_get_direct_mem_ptr(MODULE* mod,
                                   dmi_cb cb)
  {
    //if our export hasn't been bound yet (due to a hierarch binding)
    //  we bind it now.
    //We do that here as the user of the target port HAS to bind at least on callback,
    //otherwise the socket was useless. Nevertheless, the target socket may still
    // stay unbound afterwards.
    if (!sc_core::sc_export<tlm::tlm_fw_transport_if<TYPES> >::get_interface())
      sc_core::sc_export<tlm::tlm_fw_transport_if<TYPES> >::bind(m_dummy);

    //warn if there already is a callback
    if (!m_dmi_f.empty()){
      display_warning("DMI callback already registered.");
      return;
    }

    //set the functor
    m_dmi_f.set_function(mod, cb);
  }


  //Override virtual functions of the tlm_target_socket:
  // this function is called whenever an sc_port (as part of a init socket)
  //  wants to bind to the export of the underlying tlm_target_socket
  //At this time a callback binder is created an returned to the sc_port
  // of the init socket, so that it binds to the callback binder
  virtual tlm::tlm_fw_transport_if<TYPES>& get_base_interface()
  {
    //error if this socket is already bound hierarchically
    if (m_hierarch_bind) display_error("Socket already bound hierarchically.");

    m_binders.push_back(new callback_binder_fw<TYPES>(m_binders.size()));
    return *m_binders[m_binders.size()-1];
  }

  // const overload not allowed for multi-sockets
  virtual const tlm::tlm_fw_transport_if<TYPES>& get_base_interface() const
  {
    display_error("'get_base_interface()' const not allowed for multi-sockets.");
    return base_type::get_base_interface();
  }

  //just return the export of the underlying tlm_target_socket in case of a hierarchical bind
  virtual sc_core::sc_export<tlm::tlm_fw_transport_if<TYPES> >& get_base_export()
  {
    return *this;
  }

  //just return the export of the underlying tlm_target_socket in case of a hierarchical bind
  virtual const sc_core::sc_export<tlm::tlm_fw_transport_if<TYPES> >& get_base_export() const
  {
    return base_type::get_base_export();
  }

  //the standard end of elaboration callback
  void end_of_elaboration(){
    //'break' here if the socket was told not to do callback binding
    if (m_eoe_disabled) return;

    //get the callback binders and the multi binds of the top of the hierachical bind chain
    // NOTE: this could be the same socket if there is no hierachical bind
    std::vector<callback_binder_fw<TYPES>* >& binders=get_hierarch_bind()->get_binders();
    std::map<unsigned int, tlm::tlm_bw_transport_if<TYPES>*>&  multi_binds=get_hierarch_bind()->get_multi_binds();

    //iterate over all binders
    for (unsigned int i=0; i<binders.size(); i++) {
      binders[i]->set_callbacks(m_nb_f, m_b_f, m_dmi_f, m_dbg_f); //set the callbacks for the binder
      if (multi_binds.find(i)!=multi_binds.end()) //check if this connection is multi-multi
        //if so remember the interface
        m_sockets.push_back(multi_binds[i]);
      else{ //if we are bound to a normal socket
        //get the calling port and try to cast it into a tlm socket base
        base_initiator_socket_type* test=dynamic_cast<base_initiator_socket_type*>(binders[i]->get_other_side());
        if (!test){display_error("Not bound to tlm_socket.");}
        m_sockets.push_back(&test->get_base_interface()); //remember the interface
      }
    }
  }

  //
  // Bind multi target socket to multi target socket (hierarchical bind)
  //
  virtual void bind(base_type& s)
  {
    //warn if already bound hierarchically
    if (m_eoe_disabled){
      display_warning("Socket already bound hierarchically. Bind attempt ignored.");
      return;
    }

    //disable our own end of elaboration call
    disable_cb_bind();

    //inform the bound target socket that it is bound hierarchically now
    s.set_hierarch_bind((base_type*)this);
    base_type::bind(s); //satisfy SystemC
  }

  //operator notation for hierarchical bind
  void operator() (base_type& s)
  {
    bind(s);
  }

  //get access to sub port
  tlm::tlm_bw_transport_if<TYPES>* operator[](int i){return m_sockets[i];}

  //get number of bound initiators
  // NOTE: this is only valid at end of elaboration!
  unsigned int size(){return get_hierarch_bind()->get_binders().size();}

protected:
  //implementation of base class interface
  base_type* get_hierarch_bind(){if (m_hierarch_bind) return m_hierarch_bind->get_hierarch_bind(); else return this;}
  std::map<unsigned int, tlm::tlm_bw_transport_if<TYPES>*>&  get_multi_binds(){return m_multi_binds;}
  void set_hierarch_bind(base_type* h){m_hierarch_bind=h;}
  tlm::tlm_fw_transport_if<TYPES>* get_last_binder(tlm::tlm_bw_transport_if<TYPES>* other){
    m_multi_binds[m_binders.size()-1]=other;
    return m_binders[m_binders.size()-1];
  }

  //map that stores to which index a multi init socket is connected
  // and the interface of the multi init socket
  std::map<unsigned int, tlm::tlm_bw_transport_if<TYPES>*> m_multi_binds;

  void disable_cb_bind(){ m_eoe_disabled=true;}
  std::vector<callback_binder_fw<TYPES>* >& get_binders(){return m_binders;}
  //vector of connected sockets
  std::vector<tlm::tlm_bw_transport_if<TYPES>*> m_sockets;
  //vector of binders that convert untagged interface into tagged interface
  std::vector<callback_binder_fw<TYPES>*> m_binders;

  base_type*  m_hierarch_bind; //pointer to hierarchical bound multi port
  bool m_eoe_disabled; //bool that diables callback bindings at end of elaboration
  callback_binder_fw<TYPES> m_dummy; //a dummy to bind to the export

  //callbacks as functors
  // (allows to pass the callback to another socket that does not know the type of the module that owns
  //  the callbacks)
  typename callback_binder_fw<TYPES>::nb_func_type    m_nb_f;
  typename callback_binder_fw<TYPES>::b_func_type     m_b_f;
  typename callback_binder_fw<TYPES>::debug_func_type m_dbg_f;
  typename callback_binder_fw<TYPES>::dmi_func_type   m_dmi_f;
};

}

#endif
