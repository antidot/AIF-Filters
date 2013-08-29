import java.io.PrintWriter;
import java.io.StringWriter;
import java.sql.Connection;

import net.antidot.paf.Configuration;
import net.antidot.paf.Document;
import net.antidot.paf.GeneratorFilter;
import net.antidot.paf.Handle;
import net.antidot.paf.Pipe;
import net.antidot.protobuf.event.EventProtos;
import net.antidot.protobuf.paf.layer.LayerType.Type;
import net.antidot.semantic.rdf.model.impl.sesame.SesameDataSet;
import net.antidot.semantic.rdf.rdb2rdf.dm.core.DirectMapper;
import net.antidot.semantic.rdf.rdb2rdf.dm.core.DirectMappingEngine.Version;
import net.antidot.semantic.rdf.rdb2rdf.r2rml.core.R2RMLProcessor;
import net.antidot.sql.model.core.DriverType;
import net.antidot.sql.model.core.SQLConnector;

import org.openrdf.repository.RepositoryException;
import org.openrdf.rio.RDFFormat;

/**
 * @author Laurent Mazuel
 * 
 */
public class Db2TripleFilter extends GeneratorFilter {

    private enum Rdb2RdfMode {
	DirectMapping, R2RML
    }

    private Rdb2RdfMode mode = null;

    private String login = null;
    private String password = null;
    private String url = null;

    private DriverType driver = null;

    private String base_uri = null;

    private String r2rml_file = null;

    private String nativestore_path;

    private boolean create_doc_with_base_uri = false;
    private Type output_layer = null;
    private RDFFormat rdf_format = null;

    /**
     * @param configuration
     * @param handle
     */
    public Db2TripleFilter(Configuration configuration, Handle handle) {
	super(configuration, handle);
    }

    /*
     * (non-Javadoc)
     * 
     * @see net.antidot.paf.GeneratorFilter#init()
     */
    @Override
    public void init() throws Exception {
	// May throw IllegalArgumentException if not valid
	mode = Rdb2RdfMode.valueOf(getStringOrThrow("rdb2rdf_mode"));
	handle.log(EventProtos.Type.INFO, "Use Rdb2Rdf mode: " + mode.name(),
		EventProtos.Level.DEBUG);

	login = getStringOrThrow("login");
	handle.log(EventProtos.Type.INFO, "Use login: " + login,
		EventProtos.Level.DEBUG);

	password = getStringOrThrow("password");
	url = getStringOrThrow("url");
	handle.log(EventProtos.Type.INFO, "Use url: " + url,
		EventProtos.Level.DEBUG);

	String strDriver = getStringOrThrow("driver");
	if (strDriver.equalsIgnoreCase(DriverType.MysqlDriver.getDriverName())) {
	    driver = DriverType.MysqlDriver;
	}
	else if (strDriver.equalsIgnoreCase(DriverType.PostgreSQL
		.getDriverName())) {
	    driver = DriverType.PostgreSQL;
	}
	else {
	    handle.log(EventProtos.Type.WARNING, "The driver " + strDriver
		    + " is NOT tested under current implementation",
		    EventProtos.Level.NORMAL);
	    driver = new DriverType(strDriver);
	}
	handle.log(EventProtos.Type.INFO,
		"Use Driver: " + driver.getDriverName(),
		EventProtos.Level.DEBUG);

	base_uri = getStringOrDefault("base_uri", "http://foo.example/DB/");
	handle.log(EventProtos.Type.INFO, "Use base uri: " + base_uri,
		EventProtos.Level.DEBUG);

	// Parametre pour dire si on sauve une couche ou un TS

	String formatStr = getStringOrDefault("rdf_format", "RDF/XML");
	rdf_format = RDFFormat.valueOf(formatStr);

	if (mode == Rdb2RdfMode.R2RML) {
	    r2rml_file = getStringOrThrow("r2rml_file");
	}

	if (configuration.has_arg("create_doc_with_base_uri")) {
	    create_doc_with_base_uri = configuration
		    .get_boolean("create_doc_with_base_uri");
	}
	output_layer = configuration.get_output_type(Type.CONTENTS);

	nativestore_path = getStringOrDefault("nativestore_path", null);
    }

    private void checkOrThrow(String paramName) {
	if (!configuration.has_arg(paramName)) {
	    throw new ConfigurationMandatoryMissingException("\"" + paramName
		    + "\" has to defined");
	}
    }

    private String getStringOrThrow(String paramName) {
	checkOrThrow(paramName);
	return configuration.get_string(paramName);
    }

    private String getStringOrDefault(String paramName, String defaultValue) {
	try {
	    return getStringOrThrow(paramName);
	}
	catch (Exception err) {
	    return defaultValue;
	}
    }

    /*
     * (non-Javadoc)
     * 
     * @see net.antidot.paf.GeneratorFilter#generate()
     */
    @Override
    public void generate() throws Exception {
	handle.log(EventProtos.Type.INFO,
		"Launch db2triples generation",
		EventProtos.Level.NORMAL);

	// Open test database
	Connection conn = null;
	// Connect database
	try {
	    conn = SQLConnector.connect(login, password, url, driver);
	}
	catch (Exception err) {
	    handle.log(EventProtos.Type.FATAL,
		    "Cannot connect to SQL database", EventProtos.Level.NORMAL,
		    stackTraceToString(err));
	    throw err;
	}

	SesameDataSet g = null;
	try {
	    if (mode == Rdb2RdfMode.R2RML) {
		g = R2RMLProcessor.convertDatabase(conn, r2rml_file, base_uri,
			nativestore_path, driver);
	    }
	    else if (mode == Rdb2RdfMode.DirectMapping) {
		g = DirectMapper.generateDirectMapping(conn,
			Version.WD_20120529, driver, base_uri, null,
			nativestore_path);
	    }
	    else {
		handle.log(EventProtos.Type.FATAL,
			"Invalid filter state, abort", EventProtos.Level.NORMAL);
	    }
	}
	catch (Exception err) {
	    handle.log(EventProtos.Type.FATAL,
		    "Something fatal happens while extracting RDF",
		    EventProtos.Level.NORMAL, stackTraceToString(err));
	    throw err;
	}

	if (create_doc_with_base_uri) {
	    String rdf = g.printRDF(rdf_format);

	    Document doc = handle.new_document(base_uri);
	    doc.set_layer(rdf, output_layer, rdf_format.getDefaultMIMEType(),
		    rdf_format.hasCharset() ? rdf_format.getCharset().name()
			    : "", "");
	}

	try {
	    g.closeRepository();
	}
	catch (RepositoryException err) {
	    handle.log(EventProtos.Type.WARNING,
		    "Cannot close Sesame repository", EventProtos.Level.NORMAL,
		    stackTraceToString(err));
	}
    }

    private static String stackTraceToString(Exception err) {
	StringWriter sw = new StringWriter();
	err.printStackTrace(new PrintWriter(sw));
	return sw.toString();
    }

    public static void main(String[] args) {
	int return_code = Pipe.start_filter(Db2TripleFilter.class, args);
	System.exit(return_code);
    }
}
