using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace RCSwitch.Example
{
    public sealed partial class MainPage : Page
    {
        RCSwitchIO rcSwitch = new RCSwitchIO(6, 5); // connect sender to GPIO6 and receiver to GPIO5

        public MainPage()
        {
            this.InitializeComponent();
            rcSwitch.OnSignalReceived += RcSwitch_OnSignalReceived;//attach the event handler for receiving signals
        }

        private void RcSwitch_OnSignalReceived(object sender, Signal signal)
        {
            System.Diagnostics.Debug.WriteLine($"received: {signal.Code} via protocol {signal.Protocol} with bitlength {signal.BitLength}");
        }

        private void On_Click(object sender, RoutedEventArgs e)
        {
            rcSwitch.Switch(codeGroupTB.Text, codeDeviceTB.Text, true);
        }
        private void Off_Click(object sender, RoutedEventArgs e)
        {
            rcSwitch.Switch(codeGroupTB.Text, codeDeviceTB.Text, true);
        }
    }
}
