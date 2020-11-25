import React from 'react';
import './App.css';
import axios from "axios"


/**
*       ssid: s,
        pass: p,
        accel_samples_max: asm,
        accel_samples_delay_s: asd,
        accel_threshold_gs: atg,
        cell_carrier: cc,
        cell_number: cn,
        sms_email:e
*/


class App extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      config:{
        s:"",
        p: "",
        asm: 10,
        asd: 10,
        atg: 2.0,
        cc: "",
        cn: "",
        e:""
    },
      configRetreived:false,
      message:""
    };
    
    this.renderConfigForm= this.renderConfigForm.bind(this);
    this.handleChange = this.handleChange.bind(this);
    this.handleSubmit = this.handleSubmit.bind(this);
    this.getConfig = this.getConfig.bind(this);
    this.setConfig = this.setConfig.bind(this);

  }

  async getConfig() {
    return new Promise(async (resolve, reject) => {
      try {
        const response = await axios.get('/getconfig')
        console.log("CONFIG RESPONSE: ", response)
        const configData = response.data
        resolve(configData);
        this.setState({message:"Config Received"})
      }
      catch (error) {
        reject(error);
      }
    })
  }

  async setConfig() {
    return new Promise(async (resolve, reject) => {
      console.log("Submiting Confing: ",this.state.config)
      try {
        let configForUpload = this.state.config;
        console.log("configForUpload: ", configForUpload );
        const response = await axios.post('/setconfig',configForUpload)
        const resultData = response.data.result
        resolve(resultData);
       
      }
      catch (error) {
        reject(error);
      }
    })
  }
  handleChange(event) {
    console.log("Handle Event : ", event.target)
    let updatedState = {...this.state}
    updatedState.config[event.target.name] = event.target.value;
    updatedState.config["e"] = `${updatedState.config.cell_number}@${updatedState.config.cell_carrier}`
    
    this.setState( updatedState);
  }

  async handleSubmit(event) {
      event.preventDefault();
   try{
      await this.setConfig();
      this.setState({message:"Config submitted"})
   }catch(error){
    this.setState({message:"Failed to Submit Settings: "+ error})
   }
  }
  renderConfigForm() {
    return <form onSubmit={this.handleSubmit}>
      <label>
        Wifi SSID:
          <input type="text" name="s" value={this.state.config.s} onChange={this.handleChange} />
      </label>

      <label>
        Wifi Password:
          <input type="password" name="p" value={this.state.config.p} onChange={this.handleChange} />
      </label>


      <label>
        Movement Samples(How many times it checks movement):
          <input type="number" name="asm"  value={this.state.config.asm} onChange={this.handleChange} />
      </label>

      <label>
        Movement Sample Interval(How much time in between each movement check):
          <input type="number" name="asd" value={this.state.config.asd} onChange={this.handleChange} />
      </label>

      <label>
        Movemnt Threshold (How much movement is needed to trigger)
          <input type="number" name="atg" value={this.state.config.atg} onChange={this.handleChange} />
      </label>

      <label>
        Cell Phone Number
          <input type="text" name="cn" value={this.state.config.cn} onChange={this.handleChange} />
      </label>

      <label>
        Cell Phone Carrier (Needed for free SMS Service)
        <select name="cc"  value = {this.state.config.cc} onChange={this.handleChange}>
          <option value="txt.att.net">AT&T</option>
          <option value="tmomail.net">T-Mobile</option>
          <option value="vtext.com">Verizon</option>
          <option value="messaging.sprintpcs.com">Sprint </option>
          {/* <option value="vtext.com">XFinity Mobile </option> */}
          <option value="vmobl.com">Virgin Mobile</option>
          <option value="mmst5.tracfone.com">Tracfone</option>
          <option value="mymetropcs.com">Metro PCS</option>
          <option value="sms.myboostmobile.com"> Boost Mobile</option>
          <option value="sms.cricketwireless.net">Cricket</option>
          <option value="text.republicwireless.com">Republic Wireless</option>
          <option value="msg.fi.google.com">Google Fi (Project Fi)</option>
          <option value="email.uscc.net">U.S. Cellular</option>
          <option value="message.ting.com">Ting</option>
          <option value="mailmymobile.net">Consumer Cellular </option>
          <option value="cspire1.com">C-Spire </option>
          {/* <option value="vtext.com">Page Plus</option> */}
        </select>
      </label>

      <input type="submit" value="Submit" />
    </form>

  }





  async componentDidMount() {

    try {
      const configResults = await this.getConfig();
      console.log("Got Config: ", configResults);
    
      const { cell_carrier,cell_number,password,sms_email,ssid,accel_samples_max,accel_samples_delay_s,accel_threshold_gs} = configResults
      if(configResults){
        this.setState({
          configRetreived: true,
          config:{cell_carrier,cell_number,password,sms_email,ssid,accel_samples_max,accel_samples_delay_s,accel_threshold_gs}
        })
      }
    }
    catch (error) {
      console.log("Failed to fetch Settings: ", error)
      this.setState({message:"Failed to fetch Settings: "+ error})
    }
  }


  render() {
    return (
      <div className="container">
        {this.renderConfigForm()}
        <p className="message">{this.state.message}</p>
      </div>
    );
  }
}

export default App;
