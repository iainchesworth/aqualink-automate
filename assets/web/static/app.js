'use strict';

class ErrorBoundary extends React.Component {
    constructor(props) {
        super(props);
        this.state = { hasError: false };
    }

    static getDerivedStateFromError(error) {
        return { hasError: true };
    }

    componentDidCatch(error, info) {
        console.error("Uncaught error:", error, info);
    }

    render() {
        if (this.state.hasError) {
            return React.createElement("div", null, "Something went wrong. Please try again later.");
        }
        return this.props.children;
    }
}

var ButtonComponent = function (props) {
    return React.createElement(
        "div",
        { key: props.id, className: "button" },
        React.createElement("label", null, props.name),
        React.createElement(
            "button",
            { onClick: props.onClick },
            props.state ? 'On' : 'Off'
        )
    );
};

var App = function () {
    var source = axios.CancelToken.source();

    var [buttons, setButtons] = React.useState([]);
    var [version, setVersion] = React.useState('');
    var [loading, setLoading] = React.useState(true);
    var [error, setError] = React.useState(null);

    var fetchButtons = function () {
        setError(null);
        setLoading(true);
        axios.get('/api/equipment/buttons', { cancelToken: source.token }).then(function (response) {
            setButtons(response.data);
            setLoading(false);
        }).catch(function (error) {
            if (axios.isCancel(error)) return;
            setError(error.message);
            setLoading(false);
        });

        axios.get('/api/equipment/version', { cancelToken: source.token }).then(function (response) {
            setVersion(response.data.version);
        }).catch(function (error) {
            if (axios.isCancel(error)) return;
            setError(error.message);
        });
    };

    React.useEffect(function () {
        fetchButtons();
        return function () {
            source.cancel();
        }
    }, []);

    var toggleButton = function (buttonId) {
        axios.post("/api/equipment/buttons/".concat(buttonId, "/toggle")).then(function (response) {
            setButtons(function (prevButtons) {
                return prevButtons.map(function (button) {
                    return button.id === buttonId ? response.data : button;
                });
            });
        }).catch(function (error) {
            setError(error.message);
        });
    };

    if (loading) return React.createElement("div", null, "Loading...");
    if (error) return React.createElement("div", null, "Error: ", error, React.createElement("button", { onClick: fetchButtons }, "Retry"));

    return React.createElement(
        "div",
        { className: "App" },
        React.createElement("h1", null, "Pool Control Panel"),
        buttons.map(function (button) {
            return React.createElement(
                ButtonComponent,
                {
                    key: button.id,
                    id: button.id,
                    name: button.name,
                    state: button.state,
                    onClick: function () { return toggleButton(button.id); }
                }
            );
        }),
        React.createElement("div", { className: "version-info" }, "Version: ", version)
    );
};

// Use createRoot for React 18
ReactDOM.createRoot(document.getElementById('app')).render(React.createElement(App, null));
