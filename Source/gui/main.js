// This function sends a message to the C++ backend
function sendToCpp(parameterID, value) {
    // The JUCE WebView provides this special function to send messages
    if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.juce) {
        const message = `${parameterID}:${value}`;
        window.webkit.messageHandlers.juce.postMessage(message);
    } else {
        // You can add a fallback for testing in a normal browser here
        console.log(`Would send: ${parameterID}:${value}`);
    }
}

// This function is called by the C++ backend to update the GUI
window.updateGUI = (parameterID, value) => {
    const slider = document.getElementById(parameterID);
    const label = document.getElementById(`${parameterID}_label`);
    if (slider) {
        slider.value = value;
    }
    if (label) {
        updateLabel(label, parameterID, value);
    }
};

function updateLabel(label, id, value) {
    if (id === 'CUTOFF') {
        label.textContent = `${parseFloat(value).toFixed(0)} Hz`;
    } else if (id === 'GAIN') {
        label.textContent = `${parseFloat(value).toFixed(1)} dB`;
    } else {
        label.textContent = parseFloat(value).toFixed(3);
    }
}

// Add event listeners to all sliders
document.querySelectorAll('.slider').forEach(slider => {
    slider.addEventListener('input', (event) => {
        const id = event.target.id;
        const value = event.target.value;
        
        // Send the new value to C++
        sendToCpp(id, value);
        
        // Update the text label next to the slider
        const label = document.getElementById(`${id}_label`);
        if (label) {
            updateLabel(label, id, value);
        }
    });
});