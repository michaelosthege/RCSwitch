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

// Die Vorlage "Leere Seite" ist unter http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409 dokumentiert.

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
            StartReceiving();
        }

        private void On_Click(object sender, RoutedEventArgs e)
        {
            rcSwitch.Switch(codeGroupTB.Text, codeDeviceTB.Text, true);
        }
        private void Off_Click(object sender, RoutedEventArgs e)
        {
            rcSwitch.Switch(codeGroupTB.Text, codeDeviceTB.Text, true);
        }

        private async void StartReceiving()
        {
            await System.Threading.Tasks.Task.Delay(20);
            while (true)
            {
                if (rcSwitch.available())
                {
                    System.Diagnostics.Debug.WriteLine($"received: {rcSwitch.getReceivedCode()}");
                    rcSwitch.resetAvailable();
                }
                await System.Threading.Tasks.Task.Delay(10);
            }
        }
    }
}
