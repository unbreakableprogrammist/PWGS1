using System;
using System.Windows;
using Libraria.Models;
using Libraria.ViewModels;

namespace Libraria.Views
{
    /// <summary>
    /// Interaction logic for EditBookWindow.xaml
    /// </summary>
    public partial class EditBookWindow : Window
    {
        private readonly BookEditViewModel _viewModel;

        public EditBookWindow()
            : this(new BookEditViewModel(new Book(), true))
        {
        }

        public EditBookWindow(BookEditViewModel viewModel)
        {
            InitializeComponent();
            _viewModel = viewModel;
            DataContext = _viewModel;
            _viewModel.CloseRequested += OnCloseRequested;
            Closed += OnClosed;
        }

        private void OnCloseRequested(object? sender, CloseRequestedEventArgs e)
        {
            DialogResult = e.DialogResult;
            Close();
        }

        private void OnClosed(object? sender, EventArgs e)
        {
            _viewModel.CloseRequested -= OnCloseRequested;
            Closed -= OnClosed;
        }
    }
}
