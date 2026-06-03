using System.Windows;
using Libraria.ViewModels;

namespace Libraria
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            DataContext = new MainViewModel();
        }
    }
}
