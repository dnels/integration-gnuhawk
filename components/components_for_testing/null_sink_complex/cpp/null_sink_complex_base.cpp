/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file 
 * distributed with this source distribution.
 * 
 * This file is part of GNUHAWK.
 * 
 * GNUHAWK is free software: you can redistribute it and/or modify is under the 
 * terms of the GNU General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later 
 * version.
 * 
 * GNUHAWK is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more 
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program.  If not, see http://www.gnu.org/licenses/.
 */


#include "null_sink_complex_base.h"

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY
    
 	Source: null_sink_complex.spd.xml
 	Generated on: Tue Feb 26 12:04:18 EST 2013
 	Redhawk IDE
 	Version:M.1.8.3
 	Build id: v201302151037

*******************************************************************************************/

//
//  Allow for logging 
// 
PREPARE_LOGGING(null_sink_complex_base);


inline static unsigned int
round_up (unsigned int n, unsigned int multiple)
{
  return ((n + multiple - 1) / multiple) * multiple;
}

inline static unsigned int
round_down (unsigned int n, unsigned int multiple)
{
  return (n / multiple) * multiple;
}


/******************************************************************************************

    The following class functions are for the base class for the component class. To
    customize any of these functions, do not modify them here. Instead, overload them
    on the child class

******************************************************************************************/
 
null_sink_complex_base::null_sink_complex_base(const char *uuid, const char *label) :
 GnuHawkBlock(uuid, label), 
 serviceThread(0), 
 noutput_items(0),
 _sriListener(*this), 
 _maintainTimeStamp(false),
 _throttle(false)
{
    construct();
}

void null_sink_complex_base::construct()
{
    Resource_impl::_started = false;
    loadProperties();
    serviceThread = 0;

    
    PortableServer::ObjectId_var oid;
    complex_in = new BULKIO_dataFloat_In_i("complex_in",&_sriListener );
    oid = ossie::corba::RootPOA()->activate_object(complex_in);

    registerInPort(complex_in);
}

/*******************************************************************************************
    Framework-level functions
    These functions are generally called by the framework to perform housekeeping.
*******************************************************************************************/
void null_sink_complex_base::initialize() throw (CF::LifeCycle::InitializeError, CORBA::SystemException)
{
}

void null_sink_complex_base::start() throw (CORBA::SystemException, CF::Resource::StartError)
{
    boost::mutex::scoped_lock lock(serviceThreadLock);
    if (serviceThread == 0) {
        if ( complex_in ) complex_in->unblock();
       	serviceThread = service_thread( this, 0.1);
        serviceThread->start();
    }
    
    if (!Resource_impl::started()) {
    	Resource_impl::start();
    }
}

void null_sink_complex_base::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    if ( complex_in ) complex_in->block();
 
    {
      boost::mutex::scoped_lock lock(_sriMutex);
      _sriQueue.clear();
    }

    // release the child thread (if it exists)
    if (serviceThread != 0) {
      {
        boost::mutex::scoped_lock lock(serviceThreadLock);
        LOG_TRACE( null_sink_complex_base, "Stopping Service Function" );
        serviceThread->stop();
      }

      if ( !serviceThread->release()) {
         throw CF::Resource::StopError(CF::CF_NOTSET, "Processing thread did not die");
      }

      boost::mutex::scoped_lock lock(serviceThreadLock);
      if ( serviceThread ) {
        delete serviceThread;
      }
    }
    serviceThread = 0;

    if (Resource_impl::started()) {
        Resource_impl::stop();
    }
    
    LOG_TRACE( null_sink_complex_base, "COMPLETED STOP REQUEST" );
}

CORBA::Object_ptr null_sink_complex_base::getPort(const char* _id) throw (CORBA::SystemException, CF::PortSupplier::UnknownPort)
{

    std::map<std::string, Port_Provides_base_impl *>::iterator p_in = inPorts.find(std::string(_id));
    if (p_in != inPorts.end()) {

        if (!strcmp(_id,"complex_in")) {
            BULKIO_dataFloat_In_i *ptr = dynamic_cast<BULKIO_dataFloat_In_i *>(p_in->second);
            if (ptr) {
                return BULKIO::dataFloat::_duplicate(ptr->_this());
            }
        }
    }

    std::map<std::string, CF::Port_var>::iterator p_out = outPorts_var.find(std::string(_id));
    if (p_out != outPorts_var.end()) {
        return CF::Port::_duplicate(p_out->second);
    }

    throw (CF::PortSupplier::UnknownPort());
}

void null_sink_complex_base::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{
    // This function clears the component running condition so main shuts down everything
    try {
        stop();
    } catch (CF::Resource::StopError& ex) {
        // TODO - this should probably be logged instead of ignored
    }

    // deactivate ports
    releaseInPorts();
    releaseOutPorts();

    delete(complex_in);
 
    Resource_impl::releaseObject();
    LOG_TRACE( null_sink_complex_base, "COMPLETED RELEASE OBJECT" );
}

void null_sink_complex_base::loadProperties()
{
}




void null_sink_complex_base::setupIOMappings()
{
  int ninput_streams = 0;
  int noutput_streams = 0;

  if ( !validGRBlock() ) return;
  
  ninput_streams  = gr_sptr->get_max_input_streams();
  gr_io_signature_sptr g_isig = gr_sptr->input_signature();

  //
  // RESOLVE: Still need to resolve the issue with the input port/stream to output port.  We also need to resolve issue
  // with "ganging" ports together as an input to a GNU RADIO Block. transform cplx to real ... r/i -> float
  //
  
  LOG_DEBUG( null_sink_complex_base, "GNUHAWK IO MAPPINGS IN/OUT " << ninput_streams << "/" << noutput_streams );
  std::string sid("");

  //
  // Someone reset the GR Block so we need to clean up old mappings if they exists
  // we need to reset the io signatures and check the vlens
  //
 

 
   if ( _istreams.size() > 0 ) {

 
    LOG_DEBUG(  null_sink_complex_base, "RESET INPUT SIGNATURE SIZE:" << _istreams.size() );
    IStreamList::iterator istream;
    for ( int idx=0 ; istream != _istreams.end(); idx++, istream++ ) {
        // re-add existing stream definitons
      LOG_DEBUG(  null_sink_complex_base, "ADD READ INDEX TO GNU RADIO BLOCK");
      if ( ninput_streams == -1 ) gr_sptr->add_read_index();

      // setup io signature 
      istream->associate( gr_sptr );
    }


    return;
  }


     
  // setup mapping of RH Port to GNU RADIO BLOCK input streams as a 1-1 mapping (basically we ignore streamID when pulling data from port)
  // for case ninput == -1 and 1 port we map out streamID to each GNU Radio Block input stream this is done in the notifySRI callback method
  if ( ninput_streams != -1 || ( ninput_streams == -1 && inPorts.size() > 1 ) ) {
  
    int nstreams = inPorts.size();
    if ( ninput_streams != -1 ) nstreams = std::min( nstreams, ninput_streams);
    
    RH_ProvidesPortMap::iterator p_in = inPorts.begin();
    for ( int i=0; i < nstreams && p_in != inPorts.end(); i++, p_in++ ) {
      // need to add read index to GNU Radio Block for processing, 
      if ( ninput_streams == -1 ) gr_sptr->add_read_index();
      _istreams.push_back( gr_istream< BULKIO_dataFloat_In_i > ( dynamic_cast< BULKIO_dataFloat_In_i * >(p_in->second), sid, gr_sptr, i));
    }
  }
  else {   // ninput_stream is variable == -1 and  component ports == 1
    //
    // need to worry about sync between when service function starts and pushSRI happens, 
    //
    for ( RH_ProvidesPortMap::iterator p_in = inPorts.begin(); p_in != inPorts.end(); p_in++ ) {

      BULKIO_dataFloat_In_i *port = dynamic_cast< BULKIO_dataFloat_In_i * >(p_in->second);
      BULKIO::StreamSRISequence_var sris = port->activeSRIs();
      for ( uint32_t i=0 ; i < sris->length(); i++ ) {
         BULKIO::StreamSRI sri = sris[i];
         int mode = sri.mode;
	 sid = sri.streamID;
	 _istreams.push_back( gr_istream< BULKIO_dataFloat_In_i > ( port, sid, gr_sptr, i, mode ));
      }
	  
    }
  }

     
}



void null_sink_complex_base::notifySRI( BULKIO_dataFloat_In_i *port, BULKIO::StreamSRI &sri ) {

  LOG_TRACE( null_sink_complex_base, "START NotifySRI  port:stream " << port->getName() << "/" << sri.streamID);
  boost::mutex::scoped_lock lock(_sriMutex);
  _sriQueue.push_back( std::make_pair( port, sri ) );
  LOG_TRACE( null_sink_complex_base, "END  NotifySRI  QUEUE " << _sriQueue.size() << " port:stream " << port->getName() << "/" << sri.streamID); 
  
}
 
void null_sink_complex_base::processStreamIdChanges() {

  boost::mutex::scoped_lock lock(_sriMutex);

  LOG_TRACE( null_sink_complex_base, "processStreamIDChanges QUEUE: " << _sriQueue.size()  );
  if (  _sriQueue.size() == 0 ) return;
  std::string sid("");

  if ( validGRBlock() ) {
    int max_input =  gr_sptr->get_max_input_streams();
    int n_input = gr_sptr->get_num_input_streams();
    LOG_TRACE( null_sink_complex_base, " IN_MAX=" << max_input << " N_IN:" << n_input);    

    bool var_istreams = false;
    if ( max_input == -1 && inPorts.size() == 1 ) 
       var_istreams = true;
    

    IStreamList::iterator istream;
    int idx=0;
    std::string sid("");
    int mode=0;
    SRIQueue::iterator item = _sriQueue.begin();
    
    for ( ; item != _sriQueue.end(); item++ ) {
       idx = 0;
       sid = "";
       mode= item->second.mode;
       sid = item->second.streamID;
       istream = _istreams.begin();
       for ( ; istream != _istreams.end(); idx++, istream++ ) {
	   if ( !var_istreams && istream->port == item->first ) {
	      break;
	   }
	   else if ( var_istreams && (istream->port == item->first) && (istream->streamID == sid) )  {
	        break;
	  }
       }

       if ( istream == _istreams.end() ) {
          if ( var_istreams )  {
	     if ( gr_sptr ) gr_sptr->add_read_index();
	     _istreams.push_back( gr_istream< BULKIO_dataFloat_In_i >( item->first, sid, gr_sptr, idx, mode ) );
             LOG_TRACE( null_sink_complex_base, "GR_ISTREAM::ADD  PORT:" << item->first->getName() << " STREAM_ID:" << sid << " NSTREAMS:" << _istreams.size());
    
	  }
	  else {
	       LOG_WARN( null_sink_complex_base, " NEW STREAM ID, MISSING INPUT STREAM DEFINITION"  );
	  }
       }
       else if ( !var_istreams ) {
	    LOG_DEBUG(  null_sink_complex_base,  "  SETTING GR_OSTREAM ID/STREAM_ID :" << idx << "/" << sid  );
	    istream->sri(true);
	    istream->spe(mode);
       }

	        
    }

     _sriQueue.clear();
     
  }
  else {
 	LOG_WARN(  null_sink_complex_base, " NEW STREAM ID, NO GNU RADIO BLOCK DEFINED, SRI QUEUE SIZE:" << _sriQueue.size() );
  }
      

 LOG_TRACE(  null_sink_complex_base,  "processStreamID,  GR_ISTREAM MAP SIZE:"  << _istreams.size() );

}


 

 

null_sink_complex_base::TimeDuration null_sink_complex_base::getTargetDuration() {

  TimeDuration  t_drate;;
  uint64_t samps=0;
  double   xdelta=1.0;
  double   trate=1.0;
 

  trate = samps*xdelta;
  uint64_t sec = (uint64_t)trunc(trate);
  uint64_t usec = (uint64_t)((trate-sec)*1e6);
  t_drate = boost::posix_time::seconds(sec) + 
            boost::posix_time::microseconds(usec);
  LOG_TRACE( null_sink_complex_base, " SEC/USEC " << sec << "/"  << usec << "\n"  <<
	     " target_duration " << t_drate );
  return t_drate;
}

null_sink_complex_base::TimeDuration null_sink_complex_base::calcThrottle( TimeMark &start_time,
                                             TimeMark &end_time ) {

  TimeDuration delta;
  TimeDuration target_duration = getTargetDuration();

  if ( start_time.is_not_a_date_time() == false ) {
    TimeDuration s_dtime= end_time - start_time;
    delta = target_duration - s_dtime;
    delta /= 4;
    LOG_TRACE( null_sink_complex_base, " s_time/t_dime " << s_dtime << "/" << target_duration << "\n"  <<
	      " delta " << delta );
  }
  return delta;
}



/**
  DATA ANALYZER TEMPLATE Service Function for GR_BLOCK PATTERN
*/

template < typename IN_PORT_TYPE > int null_sink_complex_base::_analyzerServiceFunction( typename  std::vector< gr_istream< IN_PORT_TYPE > > &istreams ) {

  typedef typename std::vector< gr_istream< IN_PORT_TYPE > > _IStreamList;

  boost::mutex::scoped_lock lock(serviceThreadLock);

  if ( validGRBlock() == false ) {
    
    // create our processing block
    createBlock();

    // create input/output port-stream mapping
    setupIOMappings();

    LOG_DEBUG(  null_sink_complex_base, " FINISHED BUILDING  GNU RADIO BLOCK");
  }
   
  // process any Stream ID changes this could affect number of io streams
  processStreamIdChanges();
    
  if ( !validGRBlock() || istreams.size() == 0 ) {
    LOG_WARN(null_sink_complex_base, "NO STREAMS ATTACHED TO BLOCK..." );
    return NOOP;
  }

  // resize data vectors for passing data to GR_BLOCK object
  _input_ready.resize( istreams.size() );
  _ninput_items_required.resize( istreams.size());
  _ninput_items.resize( istreams.size());
  _input_items.resize(istreams.size());
  _output_items.resize(0);
  
  //
  // RESOLVE: need to look at forecast strategy, 
  //    1)  see how many read items are necessary for N number of outputs
  //    2)  read input data and see how much output we can produce
  //
  
  //
  // Grab available data from input streams
  //
  typename _IStreamList::iterator istream = istreams.begin();
  int nitems=0;
  for ( int idx=0 ; istream != istreams.end() && serviceThread->threadRunning() ; idx++, istream++ ) {
    // note this a blocking read that can cause deadlocks
    nitems = istream->read();
    
    if ( istream->overrun() ) {
        LOG_WARN( null_sink_complex_base, " NOT KEEPING UP WITH STREAM ID:" << istream->streamID );
    }
    
    // RESOLVE issue when SRI changes that could affect the GNU Radio BLOCK
    if ( istream->sriChanged() ) {
      LOG_DEBUG( null_sink_complex_base, "SRI CHANGED, STREAMD IDX/ID: " 
               << idx << "/" << istream->pkt->streamID );
    }
  }

  LOG_TRACE( null_sink_complex_base, "READ NITEMS: "  << nitems );
  if ( nitems <= 0 && !_istreams[0].eos() ) return NOOP;

  bool exitServiceFunction = false;
  bool eos = false;
  int  nout = 0;
  while ( nout > -1 && !exitServiceFunction && serviceThread->threadRunning() ) {

    eos = false;
    nout = _forecastAndProcess( eos, istreams );
    if ( nout > -1  ) {
      // we chunked on data so move read pointer..
      istream = istreams.begin();
      for ( ; istream != istreams.end(); istream++ ) {

	int idx=std::distance( istreams.begin(), istream );
	// if we processed data for this stream
	if ( _input_ready[idx] ) {
	  size_t nitems = 0;
	  try {
	    nitems = gr_sptr->nitems_read( idx );
	  }
	  catch(...){}
      
	  istream->consume( nitems );
          LOG_TRACE( null_sink_complex_base, " CONSUME READ DATA  ITEMS/REMAIN " << nitems << "/" << istream->nitems());
	}

      }
      gr_sptr->reset_read_index();
    }

    // check for not enough data return
    if ( nout == -1 ) {

      // check for  end of stream
      istream = istreams.begin();
      for ( ; istream != istreams.end() ; istream++) if ( istream->eos() ) eos=true;

      if ( eos ) {
        LOG_TRACE( null_sink_complex_base, " DATA NOT READY, EOS:" << eos );
	_forecastAndProcess( eos, istreams );
      }

      exitServiceFunction = true;
    }

  }

  if ( eos ) {

    istream = istreams.begin();
    for ( ; istream != istreams.end() ; ) {
      int idx=std::distance( istreams.begin(), istream );
      if (  istream->eos() || eos == true ) {
        LOG_TRACE( null_sink_complex_base, " CLOSING INPUT STREAM IDX:" << idx );
	istream->close();

	// variable list of inputs then we need to close that stream
	if ( gr_sptr->get_max_input_streams() == -1 ) {
	  LOG_TRACE( null_sink_complex_base," REMOVE VARIABLE INPUT STREAM IDX:" << idx );
	  istream=istreams.erase( istream );
	  gr_sptr->remove_read_index( idx );
	}
	else {
	  istream++;
	}
      }

    }
  }

  //
  // set the read pointers of the GNU Radio Block to start at the beginning of the 
  // supplied buffers
  //
  gr_sptr->reset_read_index();

  LOG_TRACE( null_sink_complex_base, " END OF ANALYZER SERVICE FUNCTION....." << noutput_items );

  if ( nout == -1 && eos == false )
    return NOOP; 
  else
    return NORMAL;
}


template <  typename IN_PORT_TYPE > int null_sink_complex_base::_forecastAndProcess( bool &eos, typename  std::vector< gr_istream< IN_PORT_TYPE > > &istreams )
{
  typedef typename std::vector< gr_istream< IN_PORT_TYPE > >   _IStreamList;

  typename _IStreamList::iterator istream = istreams.begin();
  int nout = 0;
  bool dataReady = false;
  if ( !eos ) {
    uint64_t max_items_avail = 0;
    for ( int idx=0 ; istream != istreams.end() && serviceThread->threadRunning() ; idx++, istream++ ) {
      LOG_TRACE(  null_sink_complex_base, "GET MAX ITEMS: STREAM:"<< idx << " NITEMS/SCALARS:" << istream->nitems() << "/" << istream->_data.size() );
      max_items_avail = std::max( istream->nitems(), max_items_avail );
    }

    //
    // calc number of output items to produce
    //
    noutput_items = (int) (max_items_avail * gr_sptr->relative_rate ());
    noutput_items = round_down (noutput_items, gr_sptr->output_multiple ());

    if ( noutput_items <= 0  ) {
       LOG_TRACE( null_sink_complex_base, "DATA CHECK - MAX ITEMS  NOUTPUT/MAX_ITEMS:" <<   noutput_items << "/" << max_items_avail);
       return -1;
    }

    if ( gr_sptr->fixed_rate() ) {
      istream = istreams.begin();
      for ( int i=0; istream != istreams.end(); i++, istream++ ) {
	if ( gr_sptr->fixed_rate() ) {
	  int t_noutput_items = gr_sptr->fixed_rate_ninput_to_noutput( istream->nitems() );
	  if ( gr_sptr->output_multiple_set() ) {
	    t_noutput_items = round_up(t_noutput_items, gr_sptr->output_multiple());
	  }
	  if ( t_noutput_items > 0 ) {
	    if ( noutput_items == 0 ) noutput_items = t_noutput_items;
	    if ( t_noutput_items <= noutput_items ) noutput_items = t_noutput_items;
	  }
	}
      }
      LOG_TRACE( null_sink_complex_base, " FIXED FORECAST NOUTPUT/output_multiple == " << noutput_items  << "/" << gr_sptr->output_multiple());
    }

    //
    // ask the block how much input they need to produce noutput_items...
    // if enough data is available to process then set the dataReady flag
    //
    int32_t  outMultiple = gr_sptr->output_multiple();
    while ( !dataReady && noutput_items >= outMultiple  ) {

      //
      // ask the block how much input they need to produce noutput_items...
      //
      gr_sptr->forecast(noutput_items, _ninput_items_required);

      LOG_TRACE( null_sink_complex_base, "--> FORECAST IN/OUT " << _ninput_items_required[0]  << "/" << noutput_items  );

      istream = istreams.begin();
      uint32_t dr_cnt=0;
      for ( int idx=0 ; noutput_items > 0 && istream != istreams.end(); idx++, istream++ ) {
	// check if buffer has enough elements
	_input_ready[idx] = false;
	if ( istream->nitems() >= (uint64_t)_ninput_items_required[idx] ) {
	  _input_ready[idx] = true;
	  dr_cnt++;
	}
	LOG_TRACE( null_sink_complex_base, "ISTREAM DATACHECK NELMS/NITEMS/REQ/READY:" <<   istream->nelems() << "/" << istream->nitems() << "/" << _ninput_items_required[idx] << "/" << _input_ready[idx]);
      }
    
      if ( dr_cnt < istreams.size() ) {
        if ( outMultiple > 1 )
       	  noutput_items -= outMultiple;
        else
          noutput_items /= 2;
      }
      else {
        dataReady = true;
      }
      LOG_TRACE( null_sink_complex_base, " TRIM FORECAST NOUTPUT/READY " << noutput_items << "/" << dataReady );
    }

    // check if data is ready...
    if ( !dataReady ) {
      LOG_TRACE( null_sink_complex_base, "DATA CHECK - NOT ENOUGH DATA  AVAIL/REQ:" <<   _istreams[0].nitems() << "/" << _ninput_items_required[0] );
      return -1;	 
    }


    // reset looping variables
    int  ritems = 0;
    int  nitems = 0;

    // reset caching vectors
    _output_items.clear();
    _input_items.clear();
    _ninput_items.clear();
    istream = istreams.begin();
    for ( int idx=0 ; istream != istreams.end(); idx++, istream++ ) {

      // check if the stream is ready
      if ( !_input_ready[idx] ) continue;
      
      // get number of items remaining
      try {
        ritems = gr_sptr->nitems_read( idx );
      }
      catch(...){
        // something bad has happened, we are missing an input stream
	LOG_ERROR( null_sink_complex_base, "MISSING INPUT STREAM FOR GR BLOCK, STREAM ID:" <<   istream->streamID );
        return -2;
      } 
    
      nitems = istream->nitems() - ritems;
      LOG_TRACE( null_sink_complex_base,  " ISTREAM: IDX:" << idx  << " ITEMS AVAIL/READ/REQ " << nitems << "/" 
		 << ritems << "/" << _ninput_items_required[idx] );
      if ( nitems >= _ninput_items_required[idx] && nitems > 0 ) {
	//remove eos checks ...if ( nitems < _ninput_items_required[idx] ) nitems=0;
        _ninput_items.push_back( nitems );
	_input_items.push_back( (const void *) (istream->read_pointer(ritems)) );
      }
    }

    nout=0;
    if ( _input_items.size() != 0 && serviceThread->threadRunning() ) {
      LOG_TRACE( null_sink_complex_base, " CALLING WORK.....N_OUT:" << noutput_items << " N_IN:" << nitems << " ISTREAMS:" << _input_items.size() << " OSTREAMS:" << _output_items.size());
      nout = gr_sptr->general_work( noutput_items, _ninput_items, _input_items, _output_items);

       // sink/analyzer patterns do not return items, so consume_each is not called in Gnu Radio BLOCK
       if ( nout == 0 ) {
           gr_sptr->consume_each(nitems);
       }

      LOG_TRACE( null_sink_complex_base, "RETURN  WORK ..... N_OUT:" << nout);
    }

    // check for stop condition from work method
    if ( nout < gr_block::WORK_DONE ) {
      LOG_WARN( null_sink_complex_base, "WORK RETURNED STOP CONDITION..." << nout );
      nout=0;
      eos = true;
    }
  }

  return nout;
     
}







