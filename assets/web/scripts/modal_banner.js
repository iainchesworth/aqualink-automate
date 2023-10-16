function showBanner(headline, description) {
    // Set the headline text
    document.getElementById('toggleBannerLabel').innerText = headline;

    // Set the description text
    document.getElementById('toggleBannerDescription').innerText = description;

    // Use Bootstrap's modal function to show the banner
    var myModal = new bootstrap.Modal(document.getElementById('toggleBanner'));
    myModal.show();
}

function hideBanner() {
    // Use Bootstrap's modal function to hide the banner
    var myModal = new bootstrap.Modal(document.getElementById('toggleBanner'));
    myModal.hide();
}
