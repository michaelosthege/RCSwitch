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
using RCSwitch;
using System.Threading.Tasks;

namespace RCSwitch.Test
{
    /// <summary>
    /// Eine leere Seite, die eigenständig verwendet oder zu der innerhalb eines Rahmens navigiert werden kann.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        RCSwitchIO rcSwitch = new RCSwitchIO(6, 5); // connect sender to GPIO6 and receiver to GPIO5

        public MainPage()
        {
            this.InitializeComponent();
            TestAsync();
            rcSwitch.OnSignalReceived += RcSwitch_OnSignalReceived;
        }

        private void RcSwitch_OnSignalReceived(object sender, Signal signal)
        {
            System.Diagnostics.Debug.WriteLine($"received: {signal.Code}");
        }

        private async void TestAsync()
        {
            System.Diagnostics.Debug.WriteLine("starting test");
            for (int i = 0; i < 10; i++)
            {
                System.Diagnostics.Debug.WriteLine(rcSwitch.Switch("11001", "10000", true));
                await Task.Delay(1000);
                rcSwitch.Switch("11001", "10000", false);
                await Task.Delay(1000);
            }
            System.Diagnostics.Debug.WriteLine("test ended");
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
