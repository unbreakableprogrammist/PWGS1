using System.Windows;
using Libraria.ViewModels;

namespace Libraria.Views
{
    public partial class EditBookWindow : Window
    {
        public EditBookWindow(BookEditViewModel viewModel)
        {
            InitializeComponent();
            DataContext = viewModel;
        }
    }
}
